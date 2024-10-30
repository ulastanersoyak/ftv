#include "video/video.hpp"
#include "crypto/checksum.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include "video/video_error.hpp"
#include <functional>
#include <string>

namespace ftv
{

video::video (const file &file, const secure_key &key, resolution res,
              std::size_t fps)
    : res_{ res }, fps_{ fps }
{
  if (res.x == 0 || res.y == 0 || res.x > MAX_RESOLUTION
      || res.y > MAX_RESOLUTION)
    {
      throw video_error ("invalid resolution",
                         video_error_code::invalid_resolution);
    }

  if (fps == 0 || fps > MAX_FPS)
    {
      throw video_error ("invalid fps", video_error_code::invalid_fps);
    }

  const auto encrypted_file = encrypt (file, key);
  if (!encrypted_file)
    {
      throw video_error ("failed to encrypt file",
                         video_error_code::encryption_failed);
    }

  const auto serialized_data = serialize_encrypted_data (*encrypted_file);
  if (!serialized_data)
    {
      throw video_error ("failed to serialize file",
                         video_error_code::serialization_failed);
    }
  this->data_ = *serialized_data;

  this->metadata_.filesize = this->data_.size ();
  this->metadata_.filename = file.path ().filename ().string ();
  this->metadata_.checksum = hash (this->data_);
}

} // namespace ftv
