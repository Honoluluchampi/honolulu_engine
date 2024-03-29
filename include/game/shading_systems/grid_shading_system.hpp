#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::game {

using grid_comp = dummy_renderable_comp<utils::shading_type::GRID>;

DEFINE_SHADING_SYSTEM(grid_shading_system, grid_comp)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR_DECL(grid_shading_system, grid_comp);
    void render(const utils::graphics_frame_info& frame_info);
    void setup();
};

} // namespace hnll::game