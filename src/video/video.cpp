#include "video/video.hpp"
#include "ecc/reed_solomon.hpp"
#include "video/header.hpp"
#include "video/video_io.hpp"

#include <algorithm>
#include <cstdint>
#include <new>
#include <optional>

#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace ftv
{

namespace
{

inline constexpr double WHITE_THRESHOLD{ 128.0 };

// a header's worth of bytes is read before payload_size can be trusted, so
// reads are reserved in steps of at most this much
inline constexpr std::size_t MAX_RESERVE{ 1UZ << 20 };

std::unexpected<std::error_code>
fail (std::errc error)
{
  return std::unexpected (std::make_error_code (error));
}

[[nodiscard]] cv::Rect
cell_area (std::size_t cell_x, std::size_t cell_y, std::size_t block_size,
           std::size_t inset)
{
  return { static_cast<std::int32_t> (cell_x * block_size + inset),
           static_cast<std::int32_t> (cell_y * block_size + inset),
           static_cast<std::int32_t> (block_size - 2 * inset),
           static_cast<std::int32_t> (block_size - 2 * inset) };
}

// reads the video one cell (one bit) at a time, moving to the next frame
// when the current one is used up
class cell_reader
{
public:
  cell_reader (const std::filesystem::path &path, std::size_t block_size)
      : reader_{ path }, block_size_{ block_size },
        // compression smears the edges of a block into its neighbours, so
        // larger blocks are sampled without their outer pixels
        inset_{ block_size >= 4 ? 1UZ : 0UZ }
  {
  }

  [[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
  read (std::size_t count)
  {
    std::vector<std::byte> bytes{};
    bytes.reserve (std::min (count, MAX_RESERVE));
    for (std::size_t i = 0; i < count; ++i)
      {
        std::uint8_t value = 0;
        for (std::size_t bit = 0; bit < 8; ++bit)
          {
            const auto cell = this->next_cell ();
            if (!cell)
              {
                return fail (std::errc::result_out_of_range);
              }
            value = static_cast<std::uint8_t> ((value << 1) | (*cell ? 1 : 0));
          }
        bytes.push_back (std::byte{ value });
      }
    return bytes;
  }

private:
  [[nodiscard]] bool
  next_frame ()
  {
    cv::Mat frame{};
    if (!this->reader_.get ().read (frame) || frame.empty ())
      {
        return false;
      }
    if (frame.channels () == 1)
      {
        this->gray_ = frame;
      }
    else
      {
        cv::cvtColor (frame, this->gray_, cv::COLOR_BGR2GRAY);
      }

    this->columns_
        = static_cast<std::size_t> (this->gray_.cols) / this->block_size_;
    this->cells_
        = this->columns_
          * (static_cast<std::size_t> (this->gray_.rows) / this->block_size_);
    this->cell_ = 0;
    return this->cells_ > 0;
  }

  [[nodiscard]] std::optional<bool>
  next_cell ()
  {
    if (this->cell_ >= this->cells_ && !this->next_frame ())
      {
        return std::nullopt;
      }
    const auto area = cell_area (this->cell_ % this->columns_,
                                 this->cell_ / this->columns_,
                                 this->block_size_, this->inset_);
    ++this->cell_;
    return cv::mean (this->gray_ (area))[0] >= WHITE_THRESHOLD;
  }

  video_reader reader_;
  std::size_t block_size_;
  std::size_t inset_;
  cv::Mat gray_{};
  std::size_t columns_{ 0 };
  std::size_t cells_{ 0 };
  std::size_t cell_{ 0 };
};

} // namespace

[[nodiscard]] std::error_code
write_video (const std::filesystem::path &path,
             std::span<const std::byte> data,
             const video_options &options) noexcept
{
  try
    {
      if (std::ranges::find (BLOCK_SIZES, options.block_size)
              == BLOCK_SIZES.end ()
          || options.fps == 0 || data.size () > MAX_PAYLOAD_SIZE)
        {
          return std::make_error_code (std::errc::invalid_argument);
        }

      const std::size_t block = options.block_size;
      const std::size_t columns = options.width / block;
      const std::size_t cells_per_frame = columns * (options.height / block);
      if (cells_per_frame == 0)
        {
          return std::make_error_code (std::errc::invalid_argument);
        }

      auto stream = encode_header ({ block, data.size () });
      const auto body = rs_encode (data);
      stream.insert (stream.end (), body.begin (), body.end ());

      video_writer writer{ path };
      writer.initialize (options.fps, { options.width, options.height });

      const std::size_t total_bits = stream.size () * 8;
      for (std::size_t bit = 0; bit < total_bits;)
        {
          // unused cells at the end of the last frame stay black
          cv::Mat frame (static_cast<std::int32_t> (options.height),
                         static_cast<std::int32_t> (options.width), CV_8UC3,
                         cv::Scalar::all (0));

          for (std::size_t cell = 0;
               cell < cells_per_frame && bit < total_bits; ++cell, ++bit)
            {
              const auto value
                  = std::to_integer<std::uint8_t> (stream[bit / 8]);
              if ((value >> (7 - bit % 8)) & 1)
                {
                  frame (cell_area (cell % columns, cell / columns, block, 0))
                      .setTo (cv::Scalar::all (255));
                }
            }
          writer.get ().write (frame);
        }
      return {};
    }
  catch (const std::bad_alloc &)
    {
      return std::make_error_code (std::errc::not_enough_memory);
    }
  catch (...)
    {
      return std::make_error_code (std::errc::io_error);
    }
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
read_video (const std::filesystem::path &path) noexcept
{
  try
    {
      // the block size is only known once the header is read, so try each
      // one until a valid header shows up
      for (const auto block_size : BLOCK_SIZES)
        {
          cell_reader reader{ path, block_size };
          const auto raw_header = reader.read (ENCODED_HEADER_SIZE);
          if (!raw_header)
            {
              continue;
            }

          const auto decoded = decode_header (*raw_header);
          if (!decoded)
            {
              if (decoded.error () == std::errc::not_supported)
                {
                  return std::unexpected (decoded.error ());
                }
              continue;
            }
          if (decoded->block_size != block_size)
            {
              continue;
            }

          const auto body
              = reader.read (rs_encoded_size (decoded->payload_size));
          if (!body)
            {
              return std::unexpected (body.error ());
            }
          return rs_decode (*body, decoded->payload_size);
        }
      return fail (std::errc::bad_message);
    }
  catch (const std::bad_alloc &)
    {
      return fail (std::errc::not_enough_memory);
    }
  catch (...)
    {
      return fail (std::errc::io_error);
    }
}

} // namespace ftv
