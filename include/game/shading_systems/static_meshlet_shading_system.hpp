#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::graphics { class desc_set; }

namespace hnll::game {

using static_meshlet_comp = renderable_comp<utils::shading_type::MESHLET>;

DEFINE_SHADING_SYSTEM(static_meshlet_shading_system, static_meshlet_comp)
{
  public:
    void render(const utils::frame_info& frame_info);
    void setup();
  private:
    void setup_task_desc();
    u_ptr<graphics::desc_set> task_desc_sets_;
};

} // namespace hnll::game