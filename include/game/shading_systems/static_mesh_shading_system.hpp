#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::game {

using static_mesh_comp = renderable_comp<graphics::static_mesh>;

DEFINE_SHADING_SYSTEM(static_mesh_shading_system, static_mesh_comp)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR_DECL(static_mesh_shading_system, static_mesh_comp);
    void render(const utils::graphics_frame_info& frame_info);
    void setup();
};

} // namespace hnll::game