#pragma once

// hnll
#include <game/component.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::game {

// accepts MESH or MESHLET
template <utils::shading_type type>
DEFINE_TEMPLATE_COMPONENT(static_model_comp, type)
{
  public:
    static_model_comp();
  private:
    utils::shading_type type_ = type;
};

template <>
class static_model_comp<utils::shading_type::MESH>
{
  private:
};

} // namespace hnll::game