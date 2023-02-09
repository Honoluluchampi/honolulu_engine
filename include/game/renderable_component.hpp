#pragma once

// hnll
#include <game/shading_template_config.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <concepts>

namespace hnll::game {

template <GraphicalModel M>
class renderable_comp
{
  public:
    void bind() { model_->bind(); }
    void draw() { model_->draw(); }

    // getter
    utils::shading_type get_shading_type() const { return model_->get_shading_type(); }

  private:
    // reference to game::graphical_model_map
    M& model_;
};

} // namespace hnll::game