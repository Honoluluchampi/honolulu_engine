#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <string>

namespace hnll::graphics {

class device;
class buffer;

class texture_image
{
  public:
    u_ptr<texture_image> create(device& device, const std::string& filepath);

    explicit texture_image(device& device_, const std::string& filepath);

  private:
    device& device_;
    u_ptr<buffer> texture_buffer_;
};

} // namespace hnll::graphics