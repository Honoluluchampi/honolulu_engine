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

    static void set_target(fdtd2_field* target);
    static void remove_target(uint32_t field_id);

  private:
    static fdtd2_field* target_;
};

} // namespace hnll::physics