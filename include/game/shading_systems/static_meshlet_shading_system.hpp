#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>
#include <graphics/graphics_models/static_meshlet.hpp>

namespace hnll::game {

using static_meshlet_comp = renderable_comp<graphics::static_meshlet>;

DEFINE_SHADING_SYSTEM(static_meshlet_shading_system, static_meshlet_comp)
{
  public:
    void render(const utils::graphics_frame_info& frame_info);
    void setup();
  private:
    void setup_task_desc();

    u_ptr<graphics::desc_sets> task_desc_sets_;
    s_ptr<graphics::desc_pool> task_desc_pool_;
};

} // namespace hnll::game