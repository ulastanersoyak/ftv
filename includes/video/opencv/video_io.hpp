#include "video/resolution.hpp"

#include <filesystem>
#include <format>

#include <opencv2/opencv.hpp>

namespace ftv
{

template <typename T>
concept VideoIO
    = std::same_as<T, cv::VideoCapture> || std::same_as<T, cv::VideoWriter>;

template <typename T>
requires VideoIO<T>
class video_io
{
public:
  explicit video_io (const std::filesystem::path &path) : path_ (path)
  {
    if constexpr (std::same_as<T, cv::VideoCapture>)
      {
        io_ = T (path.string ());
        if (!io_.isOpened ())
          {
            throw std::runtime_error (
                std::format ("failed to open video file: {}", path.string ()));
          }
      }
  }

  void
  initialize (std::size_t fps, const resolution &res)
  requires std::same_as<T, cv::VideoWriter>
  {
    io_.set (cv::VIDEOWRITER_PROP_QUALITY, 100);
    io_.open (path_.string (), cv::VideoWriter::fourcc ('M', 'J', 'P', 'G'),
              static_cast<double> (fps),
              cv::Size (static_cast<std::int32_t> (res.x),
                        static_cast<std::int32_t> (res.y)));

    if (!io_.isOpened ())
      {
        throw std::runtime_error (
            std::format ("failed to open video writer: {}", path_.string ()));
      }
  }

  ~video_io ()
  {
    if (io_.isOpened ())
      {
        io_.release ();
      }
  }

  video_io (video_io &&other) noexcept
      : io_{ std::move (other.io_) }, path_{ std::move (other.path_) }
  {
    other.io_ = T ();
  }

  video_io &
  operator= (video_io &&other) noexcept
  {
    if (this != &other)
      {
        if (io_.isOpened ())
          {
            io_.release ();
          }
        io_ = std::move (other.io_);
        path_ = std::move (other.path_);
        other.io_ = T ();
      }
    return *this;
  }

  [[nodiscard]] T &
  get ()
  {
    return io_;
  }

  video_io (const video_io &) = delete;
  video_io &operator= (const video_io &) = delete;

private:
  T io_{};
  std::filesystem::path path_{};
};

using video_reader = video_io<cv::VideoCapture>;
using video_writer = video_io<cv::VideoWriter>;

} // namespace ftv
