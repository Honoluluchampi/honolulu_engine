#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::physics {

class mass_spring_cloth;

DEFINE_SHADING_SYSTEM(cloth_compute_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>)
{
  public:
    void render(const utils::graphics_frame_info& frame_info);
    void setup();

    void add_cloth()

  private:
    std::vector<mass_spring_cloth> cloth_list_;
};

} // namespace hnll::physics