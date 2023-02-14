#pragma once

// hnll
#include <game/concepts.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <concepts>

namespace hnll::game {

// renderable component id
using rc_id = uint32_t;

template <GraphicsModel M>
class renderable_comp
{
  public:
    renderable_comp(M& model) : model_(model) {}

    // hold only by its actor
    static u_ptr<renderable_comp<M>> create(M& model)
    { return std::make_unique<M>(model); }

    void bind() { model_->bind(); }
    void draw() { model_->draw(); }

    // getter
    rc_id get_rc_id() const { return rc_id_; }
    utils::shading_type get_shading_type() const { return model_->get_shading_type(); }

  private:
    // reference to game::graphical_model_map
    M& model_;
    rc_id rc_id_;
};

} // namespace hnll::game