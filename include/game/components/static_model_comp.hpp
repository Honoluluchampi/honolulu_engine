#pragma once

// hnll
#include <game/component.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

namespace graphics { class static_mesh; class static_meshlet; }

namespace game {

// accepts MESH or MESHLET
// graphics models are reference to the engine_core::model_map
template <utils::shading_type type>
DEFINE_TEMPLATE_COMPONENT(static_model_comp, type)
{
  public:
    void bind_and_draw();

    utils::shading_type get_shading_type() const { return type; }

  private:
    utils::shading_type type_ = type;
};

template <>
class static_model_comp<utils::shading_type::MESH>
{
  public:
    static_model_comp<utils::shading_type::MESH>(const std::string& model_name);

  private:
    graphics::static_mesh& model_;
};

template <>
class static_model_comp<utils::shading_type::MESHLET>
{
  public:
    static_model_comp<utils::shading_type::MESHLET>(const std::string& model_name);

  private:
    graphics::static_meshlet& model_;
};

}} // namespace hnll::game