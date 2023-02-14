#pragma once

// hnll
#include <utils/rendering_utils.hpp>

namespace hnll::graphics {

template <utils::shading_type type>
class graphics_model
{
  public:
    utils::shading_type get_shading_type() const { return type; }
};

} // namespace hnll::graphics