#pragma once

// hnll
#include <game/shading_system.hpp>

namespace hnll::game {

class grid_dummy_comp
{
  public:
    void bind() {}
    void draw() {}
    rc_id get_rc_id() const { return 0; }
    utils::shading_type get_shading_type() const { return utils::shading_type::GRID; }
};

using grid_shading_system = shading_system<grid_dummy_comp>;
}