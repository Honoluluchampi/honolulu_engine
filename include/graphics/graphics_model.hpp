#pragma once

// hnll
#include <utils/rendering_utils.hpp>
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

// std
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

namespace hnll::graphics {

class device;

template <utils::shading_type type>
class graphics_model
{
  public:
    static u_ptr<graphics_model<type>> create_from_file(device &device, const std::string &filename);

  private:
    device& device_;
};

} // namespace hnll::graphics