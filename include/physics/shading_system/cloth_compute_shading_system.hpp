#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::physics {

class mass_spring_cloth;

DEFINE_SHADING_SYSTEM(cloth_compute_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(cloth_compute_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>);
    ~cloth_compute_shading_system();

    void render(const utils::graphics_frame_info& frame_info);
    void setup();

    static void add_cloth(const s_ptr<mass_spring_cloth>& cloth);

  private:
    static std::unordered_map<uint32_t, s_ptr<mass_spring_cloth>> clothes_;
};

} // namespace hnll::physics