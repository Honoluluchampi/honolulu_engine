#pragma once

// hnll
#include <game/component.hpp>
#include <game/modules/graphics_engine.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

namespace game {

// accepts MESH or MESHLET
// graphics models are reference to the engine_core::model_map
template <utils::shading_type type>
DEFINE_TEMPLATE_COMPONENT(static_model_comp, type)
{
  public:
    template <Actor A>
    static u_ptr<static_model_comp<type>> create(const A& owner, const std::string& filename)
    {
      auto ret = std::make_unique<static_model_comp<type>>();
      ret->transform_ = owner->get_transform();
      ret->model_ = graphics_engine_core::get_graphics_model<type>(filename);
      return ret;
    }

    void bind_and_draw(VkCommandBuffer command_buffer)
    { model_->bind(command_buffer); model_->draw(command_buffer); }

    utils::shading_type get_shading_type() const { return type; }

  private:
    graphics::graphics_model<type> model_;
    // reference to the owner actor
    const utils::transform& transform_;
};

}} // namespace hnll::game