#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::physics {

class fdtd2_field;

DEFINE_SHADING_SYSTEM(fdtd2_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(fdtd2_shading_system, game::dummy_renderable_comp<utils::shading_type::CLOTH>);
    ~fdtd2_shading_system();

    void render(const utils::graphics_frame_info& frame_info);
    void setup();

  private:
    u_ptr<fdtd2_field> field_;
};

} // namespace hnll::physics