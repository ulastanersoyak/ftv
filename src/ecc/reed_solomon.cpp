#include "ecc/reed_solomon.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <new>
#include <optional>
#include <stdexcept>

// follows the decoder described in
// https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders
// (berlekamp-massey, chien search, forney) with generator 2 and fcr 0

namespace ftv
{

namespace
{

// polynomials are stored highest degree first
using poly = std::vector<std::uint8_t>;

struct gf_tables
{
  std::array<std::uint8_t, 512> exp{};
  std::array<std::uint8_t, 256> log{};
};

// GF(2^8) with primitive polynomial x^8 + x^4 + x^3 + x^2 + 1 (0x11d)
constexpr gf_tables
make_gf_tables ()
{
  gf_tables tables{};
  std::uint32_t x = 1;
  for (std::size_t i = 0; i < 255; ++i)
    {
      tables.exp[i] = static_cast<std::uint8_t> (x);
      tables.log[x] = static_cast<std::uint8_t> (i);
      x <<= 1;
      if (x & 0x100)
        {
          x ^= 0x11d;
        }
    }
  // doubled so products of two logs never need a modulo
  for (std::size_t i = 255; i < tables.exp.size (); ++i)
    {
      tables.exp[i] = tables.exp[i - 255];
    }
  return tables;
}

constexpr gf_tables GF = make_gf_tables ();

constexpr std::size_t
gf_log (std::uint8_t x)
{
  return GF.log[x];
}

constexpr std::uint8_t
gf_mul (std::uint8_t x, std::uint8_t y)
{
  if (x == 0 || y == 0)
    {
      return 0;
    }
  return GF.exp[gf_log (x) + gf_log (y)];
}

// y must not be 0
constexpr std::uint8_t
gf_div (std::uint8_t x, std::uint8_t y)
{
  if (x == 0)
    {
      return 0;
    }
  return GF.exp[gf_log (x) + 255 - gf_log (y)];
}

// x must not be 0
constexpr std::uint8_t
gf_inverse (std::uint8_t x)
{
  return GF.exp[255 - gf_log (x)];
}

// 2^power
constexpr std::uint8_t
gf_pow2 (std::size_t power)
{
  return GF.exp[power % 255];
}

poly
poly_scale (const poly &p, std::uint8_t x)
{
  poly result (p.size ());
  for (std::size_t i = 0; i < p.size (); ++i)
    {
      result[i] = gf_mul (p[i], x);
    }
  return result;
}

poly
poly_add (const poly &p, const poly &q)
{
  poly result (std::max (p.size (), q.size ()), 0);
  for (std::size_t i = 0; i < p.size (); ++i)
    {
      result[i + result.size () - p.size ()] = p[i];
    }
  for (std::size_t i = 0; i < q.size (); ++i)
    {
      result[i + result.size () - q.size ()] ^= q[i];
    }
  return result;
}

poly
poly_mul (const poly &p, const poly &q)
{
  poly result (p.size () + q.size () - 1, 0);
  for (std::size_t j = 0; j < q.size (); ++j)
    {
      for (std::size_t i = 0; i < p.size (); ++i)
        {
          result[i + j] ^= gf_mul (p[i], q[j]);
        }
    }
  return result;
}

std::uint8_t
poly_eval (const poly &p, std::uint8_t x)
{
  std::uint8_t y = p[0];
  for (std::size_t i = 1; i < p.size (); ++i)
    {
      y = gf_mul (y, x) ^ p[i];
    }
  return y;
}

const poly &
generator ()
{
  static const poly gen = []
    {
      poly result{ 1 };
      for (std::size_t i = 0; i < RS_PARITY_SIZE; ++i)
        {
          result = poly_mul (result, poly{ 1, gf_pow2 (i) });
        }
      return result;
    }();
  return gen;
}

// syndromes[0] is a placeholder so indices line up with the reference
poly
syndromes (const poly &codeword)
{
  poly result (RS_PARITY_SIZE + 1, 0);
  for (std::size_t i = 0; i < RS_PARITY_SIZE; ++i)
    {
      result[i + 1] = poly_eval (codeword, gf_pow2 (i));
    }
  return result;
}

bool
has_errors (const poly &synd)
{
  return std::ranges::any_of (synd, [] (std::uint8_t s) { return s != 0; });
}

// berlekamp-massey
std::optional<poly>
find_error_locator (const poly &synd)
{
  poly err_loc{ 1 };
  poly old_loc{ 1 };
  const std::size_t shift = synd.size () - RS_PARITY_SIZE;

  for (std::size_t i = 0; i < RS_PARITY_SIZE; ++i)
    {
      const std::size_t k = i + shift;
      std::uint8_t delta = synd[k];
      for (std::size_t j = 1; j < err_loc.size () && j <= k; ++j)
        {
          delta ^= gf_mul (err_loc[err_loc.size () - 1 - j], synd[k - j]);
        }

      old_loc.push_back (0);

      if (delta != 0)
        {
          if (old_loc.size () > err_loc.size ())
            {
              poly new_loc = poly_scale (old_loc, delta);
              old_loc = poly_scale (err_loc, gf_inverse (delta));
              err_loc = std::move (new_loc);
            }
          err_loc = poly_add (err_loc, poly_scale (old_loc, delta));
        }
    }

  const auto first_nonzero
      = std::ranges::find_if (err_loc, [] (std::uint8_t c) { return c != 0; });
  err_loc.erase (err_loc.begin (), first_nonzero);

  if (err_loc.empty () || (err_loc.size () - 1) * 2 > RS_PARITY_SIZE)
    {
      return std::nullopt;
    }
  return err_loc;
}

// chien search. err_loc_reversed is the error locator lowest degree first
std::optional<std::vector<std::size_t>>
find_error_positions (const poly &err_loc_reversed, std::size_t length)
{
  std::vector<std::size_t> positions{};
  for (std::size_t i = 0; i < length; ++i)
    {
      if (poly_eval (err_loc_reversed, gf_pow2 (i)) == 0)
        {
          positions.push_back (length - 1 - i);
        }
    }
  if (positions.size () != err_loc_reversed.size () - 1)
    {
      return std::nullopt;
    }
  return positions;
}

// forney
bool
correct_errors (poly &codeword, const poly &synd,
                const std::vector<std::size_t> &positions)
{
  std::vector<std::uint8_t> x_values{};
  poly err_loc{ 1 };
  for (const auto position : positions)
    {
      const auto x = gf_pow2 (codeword.size () - 1 - position);
      x_values.push_back (x);
      err_loc = poly_mul (err_loc, poly_add (poly{ 1 }, poly{ x, 0 }));
    }

  // error evaluator: (reversed syndromes * locator) mod x^(errors + 1)
  const poly reversed_synd (synd.rbegin (), synd.rend ());
  const poly product = poly_mul (reversed_synd, err_loc);
  const std::size_t eval_size = std::min (product.size (), err_loc.size ());
  const poly err_eval (product.end ()
                           - static_cast<std::ptrdiff_t> (eval_size),
                       product.end ());

  for (std::size_t i = 0; i < x_values.size (); ++i)
    {
      const auto x_inv = gf_inverse (x_values[i]);

      std::uint8_t loc_derivative = 1;
      for (std::size_t j = 0; j < x_values.size (); ++j)
        {
          if (j != i)
            {
              loc_derivative
                  = gf_mul (loc_derivative, 1 ^ gf_mul (x_inv, x_values[j]));
            }
        }
      if (loc_derivative == 0)
        {
          return false;
        }

      const auto y = gf_mul (x_values[i], poly_eval (err_eval, x_inv));
      codeword[positions[i]] ^= gf_div (y, loc_derivative);
    }
  return true;
}

} // namespace

[[nodiscard]] std::vector<std::byte>
rs_encode_block (std::span<const std::byte> message)
{
  if (message.size () > RS_DATA_SIZE)
    {
      throw std::invalid_argument ("reed-solomon message is too long");
    }

  const poly &gen = generator ();
  poly remainder (message.size () + gen.size () - 1, 0);
  std::ranges::transform (message, remainder.begin (), [] (std::byte b)
                            { return std::to_integer<std::uint8_t> (b); });

  // synthetic division by the generator polynomial
  for (std::size_t i = 0; i < message.size (); ++i)
    {
      const auto coef = remainder[i];
      if (coef != 0)
        {
          for (std::size_t j = 1; j < gen.size (); ++j)
            {
              remainder[i + j] ^= gf_mul (gen[j], coef);
            }
        }
    }

  std::vector<std::byte> codeword (message.begin (), message.end ());
  for (std::size_t i = message.size (); i < remainder.size (); ++i)
    {
      codeword.push_back (std::byte{ remainder[i] });
    }
  return codeword;
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
rs_decode_block (std::span<const std::byte> codeword) noexcept
{
  try
    {
      if (codeword.size () <= RS_PARITY_SIZE
          || codeword.size () > RS_CODEWORD_SIZE)
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      poly received (codeword.size ());
      std::ranges::transform (codeword, received.begin (), [] (std::byte b)
                                { return std::to_integer<std::uint8_t> (b); });

      const auto synd = syndromes (received);
      if (has_errors (synd))
        {
          const auto err_loc = find_error_locator (synd);
          if (!err_loc)
            {
              return std::unexpected (
                  std::make_error_code (std::errc::bad_message));
            }

          const poly reversed (err_loc->rbegin (), err_loc->rend ());
          const auto positions
              = find_error_positions (reversed, received.size ());
          if (!positions || !correct_errors (received, synd, *positions)
              || has_errors (syndromes (received)))
            {
              return std::unexpected (
                  std::make_error_code (std::errc::bad_message));
            }
        }

      std::vector<std::byte> message (received.size () - RS_PARITY_SIZE);
      std::ranges::transform (
          received.begin (),
          received.begin () + static_cast<std::ptrdiff_t> (message.size ()),
          message.begin (), [] (std::uint8_t b) { return std::byte{ b }; });
      return message;
    }
  catch (...)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

[[nodiscard]] std::size_t
rs_encoded_size (std::size_t data_size) noexcept
{
  return (data_size + RS_DATA_SIZE - 1) / RS_DATA_SIZE * RS_CODEWORD_SIZE;
}

[[nodiscard]] std::vector<std::byte>
rs_encode (std::span<const std::byte> data)
{
  const std::size_t codewords
      = rs_encoded_size (data.size ()) / RS_CODEWORD_SIZE;
  std::vector<std::byte> encoded (codewords * RS_CODEWORD_SIZE);
  std::vector<std::byte> message (RS_DATA_SIZE);

  for (std::size_t c = 0; c < codewords; ++c)
    {
      // the last message is padded with zeros so every codeword is full size
      std::ranges::fill (message, std::byte{ 0 });
      const std::size_t offset = c * RS_DATA_SIZE;
      std::ranges::copy (
          data.subspan (offset,
                        std::min (RS_DATA_SIZE, data.size () - offset)),
          message.begin ());

      const auto codeword = rs_encode_block (message);
      for (std::size_t i = 0; i < RS_CODEWORD_SIZE; ++i)
        {
          encoded[i * codewords + c] = codeword[i];
        }
    }
  return encoded;
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
rs_decode (std::span<const std::byte> encoded, std::size_t data_size) noexcept
{
  try
    {
      const std::size_t codewords
          = rs_encoded_size (data_size) / RS_CODEWORD_SIZE;
      if (encoded.size () != codewords * RS_CODEWORD_SIZE)
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      std::vector<std::byte> data{};
      data.reserve (codewords * RS_DATA_SIZE);
      std::vector<std::byte> codeword (RS_CODEWORD_SIZE);

      for (std::size_t c = 0; c < codewords; ++c)
        {
          for (std::size_t i = 0; i < RS_CODEWORD_SIZE; ++i)
            {
              codeword[i] = encoded[i * codewords + c];
            }
          const auto message = rs_decode_block (codeword);
          if (!message)
            {
              return std::unexpected (message.error ());
            }
          data.insert (data.end (), message->begin (), message->end ());
        }

      data.resize (data_size);
      return data;
    }
  catch (...)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

} // namespace ftv
