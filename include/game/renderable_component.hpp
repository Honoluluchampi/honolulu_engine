#pragma once

// hnll
#include <game/concepts.hpp>
#include <game/modules/graphics_engine.hpp>
#include <graphics/graphics_model.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/utils.hpp>

// std
#include <concepts>

namespace hnll::game {

// renderable component id
using rc_id = uint32_t;

template <graphics::GraphicsModel M>
class renderable_comp
{
  public:
    renderable_comp(M& model, const utils::transform& transform)
    : model_(model), transform_(transform){}

    // hold only by its actor
    template <Actor A>
    static u_ptr<renderable_comp<M>> create(A& owner, const std::string& name)
    {
      auto& model = graphics_engine_core::get_graphics_model<M>(name);
      const auto& tf = owner.get_transform();
      return std::make_unique<renderable_comp<M>>(model, tf);
    }

    template <typename... T>
    inline void bind(VkCommandBuffer cb, T... args) { model_.bind(cb, args...); }
    template <typename... T>
    inline void draw(VkCommandBuffer cb, T... args) { model_.draw(cb, args...); }

    // getter
    inline rc_id get_rc_id() const { return rc_id_; }
    inline utils::shading_type get_shading_type() const { return model_.get_shading_type(); }
    inline VkDescriptorSet get_texture_desc_set()
    { return model_.get_texture_desc_set(); }
    inline const utils::transform& get_transform() const { return transform_; }

    bool is_textured() const { return model_.is_textured(); }

  private:
    // reference to game::graphical_model_map
    M& model_;
    // reference to the owner actor
    const utils::transform& transform_;
    rc_id rc_id_;
};

} // namespace hnll::game