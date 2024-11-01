#include "video/ffmpeg/video.hpp"
#include <format>

#include <cstring>

extern "C"
{
#include <libavcodec/codec.h>
}

namespace ftv
{

video::video (const std::filesystem::path &path,
              std::span<const std::byte> bytes, const metadata &meta)
    : codec_ctx_ (AV_CODEC_ID_H264), frame_ (meta.res ()),
      format_ctx_ (path.string ()), packet_ (), metadata_ (meta)
{
  // configure codec parameters
  auto *ctx = this->codec_ctx_.get ();
  ctx->width = static_cast<std::int32_t> (meta.res ().x);
  ctx->height = static_cast<std::int32_t> (meta.res ().y);
  ctx->time_base = { 1, static_cast<int> (meta.fps ()) };
  ctx->framerate = { static_cast<int> (meta.fps ()), 1 };
  ctx->pix_fmt = AV_PIX_FMT_RGBA;

  const AVCodec *codec = avcodec_find_encoder (AV_CODEC_ID_H264);
  if (std::int32_t ret = avcodec_open2 (ctx, codec, nullptr); ret < 0)
    {
      throw std::runtime_error (
          std::format ("could not open codec AV_CODEC_ID_H264"));
    }

  AVStream *stream = avformat_new_stream (this->format_ctx_.get (), nullptr);
  if (!stream)
    {
      throw std::runtime_error (
          std::format ("could not create stream while processing {}",
                       this->metadata_.filename ()));
    }

  // stream parameters
  avcodec_parameters_from_context (stream->codecpar, ctx);
  stream->time_base = ctx->time_base;

  if (std::int32_t ret = avio_open (&format_ctx_.get ()->pb,
                                    path.string ().c_str (), AVIO_FLAG_WRITE);
      ret < 0)
    {
      throw std::runtime_error (
          std::format ("could not open output file {}", path.string ()));
    }

  // write header
  if (std::int32_t ret = avformat_write_header (format_ctx_.get (), nullptr);
      ret < 0)
    {
      throw std::runtime_error (
          std::format ("could not write header to {}", path.string ()));
    }

  // serialize metadata and combine with input bytes
  std::vector<std::byte> meta_bytes = meta.to_vec ();
  std::vector<std::byte> all_bytes;
  all_bytes.reserve (meta_bytes.size () + bytes.size ());
  all_bytes.insert (all_bytes.end (), meta_bytes.begin (), meta_bytes.end ());
  all_bytes.insert (all_bytes.end (), bytes.begin (), bytes.end ());

  const size_t bytes_per_pixel = 4; // RGBA
  const size_t row_bytes
      = static_cast<std::size_t> (ctx->width) * bytes_per_pixel;

  size_t current_byte = 0;
  size_t frame_count = 0;

  // process all bytes
  while (current_byte < all_bytes.size ())
    {
      // reset or create new frame if needed
      if (frame_count > 0)
        {
          if (auto ec = frame_.reset (); ec)
            {
              frame_ = av_frame (meta.res ());
            }
        }

      auto *frame_ptr = frame_.get ();
      size_t rows_filled = 0;

      // fill frame row by row
      while (rows_filled < static_cast<size_t> (ctx->height)
             && current_byte < all_bytes.size ())
        {
          const size_t bytes_remaining = all_bytes.size () - current_byte;
          const size_t row_bytes_to_copy
              = std::min (row_bytes, bytes_remaining);

          // copy row data
          std::memcpy (
              frame_ptr->data[0]
                  + rows_filled
                        * static_cast<std::size_t> (frame_ptr->linesize[0]),
              all_bytes.data () + current_byte, row_bytes_to_copy);

          // fill remaining pixels in row if needed
          if (row_bytes_to_copy < row_bytes)
            {
              std::memset (
                  frame_ptr->data[0]
                      + rows_filled
                            * static_cast<std::size_t> (frame_ptr->linesize[0])
                      + row_bytes_to_copy,
                  0, row_bytes - row_bytes_to_copy);
            }

          current_byte += row_bytes_to_copy;
          rows_filled++;
        }

      // fill remaining rows with zeros if any
      while (rows_filled < static_cast<size_t> (ctx->height))
        {
          std::memset (
              frame_ptr->data[0]
                  + rows_filled
                        * static_cast<std::size_t> (frame_ptr->linesize[0]),
              0, row_bytes);
          rows_filled++;
        }

      frame_ptr->pts = static_cast<std::int64_t> (frame_count++);

      // encode this frame
      std::int32_t ret
          = avcodec_send_frame (this->codec_ctx_.get (), frame_ptr);
      if (ret < 0)
        {
          throw std::runtime_error (std::format (
              "error sending frame for encoding in {}", path.string ()));
        }

      while (ret >= 0)
        {
          ret = avcodec_receive_packet (this->codec_ctx_.get (),
                                        this->packet_.get ());
          if (ret == AVERROR (EAGAIN) || ret == AVERROR_EOF)
            break;
          if (ret < 0)
            {
              throw std::runtime_error (
                  std::format ("Error receiving packet in {}",
                               this->metadata_.filename ()));
            }

          ret = av_write_frame (this->format_ctx_.get (),
                                this->packet_.get ());
          if (ret < 0)
            {
              throw std::runtime_error (
                  std::format ("error writing frame in {}", path.string ()));
            }

          if (auto ec = this->packet_.unref (); ec)
            {
              throw std::runtime_error (std::format (
                  "failed to unref packet in {}", path.string ()));
            }
        }
    }

  // flush encoder
  std::int32_t ret = avcodec_send_frame (this->codec_ctx_.get (), nullptr);
  if (ret >= 0)
    {
      while (ret >= 0)
        {
          ret = avcodec_receive_packet (this->codec_ctx_.get (),
                                        this->packet_.get ());
          if (ret == AVERROR (EAGAIN) || ret == AVERROR_EOF)
            break;
          if (ret < 0)
            {
              throw std::runtime_error (
                  std::format ("error flushing encoder in {}",
                               this->metadata_.filename ()));
            }

          ret = av_write_frame (this->format_ctx_.get (),
                                this->packet_.get ());
          if (ret < 0)
            {
              throw std::runtime_error (
                  std::format ("error writing frame during flush in {}",
                               this->metadata_.filename ()));
            }

          if (auto ec = packet_.unref (); ec)
            {
              throw std::runtime_error (
                  std::format ("Failed to unref packet in {}",
                               this->metadata_.filename ()));
            }
        }
    }

  if (av_write_trailer (format_ctx_.get ()) < 0)
    throw std::runtime_error (std::format ("Could not write trailer for {}",
                                           this->metadata_.filename ()));

  avio_closep (&format_ctx_.get ()->pb);
}

void
video::encode_frame (const av_frame &frame)
{
  std::int32_t ret = avcodec_send_frame (this->codec_ctx_.get (), frame);
  if (ret < 0)
    {
      throw std::runtime_error (
          std::format ("error sending frame for encoding in {}",
                       this->metadata_.filename ()));
    }

  while (ret >= 0)
    {
      ret = avcodec_receive_packet (this->codec_ctx_.get (),
                                    this->packet_.get ());
      if (ret == AVERROR (EAGAIN) || ret == AVERROR_EOF)
        break;
      if (ret < 0)
        {
          throw std::runtime_error (std::format (
              "error receiving packet in {}", this->metadata_.filename ()));
        }

      ret = av_write_frame (this->format_ctx_.get (), this->packet_.get ());
      if (ret < 0)
        {
          throw std::runtime_error (std::format ("error writing frame in {}",
                                                 this->metadata_.filename ()));
        }

      if (auto ec = this->packet_.unref (); ec)
        {
          throw std::runtime_error (std::format (
              "failed to unref packet in {}", this->metadata_.filename ()));
        }
    }
}

void
video::flush_encoder ()
{
  av_frame null_frame;
  this->encode_frame (null_frame); // sending nullptr signals end of stream
}

} // namespace ftv
