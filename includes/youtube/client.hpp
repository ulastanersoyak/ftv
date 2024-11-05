#pragma once

#include <string>
#include <string_view>
#include <system_error>

namespace ftv
{

class youtube_client
{
public:
  youtube_client (std::string client_id, std::string client_secret);

  youtube_client (const youtube_client &) = delete;
  youtube_client &operator= (const youtube_client &) = delete;

  youtube_client (youtube_client &&) noexcept;
  youtube_client &operator= (youtube_client &&) noexcept;

  ~youtube_client ();

  std::error_code upload (std::string_view title, std::string_view description,
                          std::string_view privacy = "private") const noexcept;

  std::error_code download (std::string_view video_id) const noexcept;

private:
  std::string client_id_{};
  std::string client_secret_{};
  std::string access_token_{};
};

}
