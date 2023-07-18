// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/modules/gui_engine.hpp>
#include <game/shading_system.hpp>
#include <utils/common_alias.hpp>

#include "fdtd12_horn.hpp"

// std
#include <iostream>

// lib
#include <AudioFile/AudioFile.h>

namespace hnll {

constexpr float dx_fdtd = 3.83e-3; // dx for fdtd grid : 3.83mm
constexpr float dt = 7.81e-6; // in seconds

constexpr float c = 340; // sound speed : 340 m/s
constexpr float rho = 1.17f;

constexpr float v_fac = dt / (rho * dx_fdtd);
constexpr float p_fac = dt * rho * c * c / dx_fdtd;

#include "common.h"

DEFINE_SHADING_SYSTEM(fdtd_wg_shading_system, fdtd_horn)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(fdtd_wg_shading_system, fdtd_horn);

    void setup()
    {
      shading_type_ = utils::shading_type::UNIQUE;

      desc_layout_ = graphics::desc_layout::create_from_bindings(device_, {fdtd_horn::common_binding_info});

      pipeline_layout_ = create_pipeline_layout<fdtd12_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        {
          desc_layout_->get_descriptor_set_layout(),
          desc_layout_->get_descriptor_set_layout(),
          desc_layout_->get_descriptor_set_layout(),
          desc_layout_->get_descriptor_set_layout(),
          desc_layout_->get_descriptor_set_layout(),
        }
      );

      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/heterogeneous_horn/shaders/spv/",
        { "fdtd_wg.vert.spv", "fdtd12.frag.spv" },
        { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
        graphics::pipeline::default_pipeline_config_info()
      );
    }

    void render(const utils::graphics_frame_info& info)
    {
      if (targets_.size() != 1)
        return;

      for (auto& target_kv : targets_) {
        auto& target = target_kv.second;

        target.update(info.frame_index);

        set_current_command_buffer(info.command_buffer);

        auto viewport_size = game::gui_engine::get_viewport_size();
        fdtd12_push push;
        push.horn_x_max = target.get_x_max();
        push.horn_y_max = target.get_y_max();
        push.pml_count = target.get_pml_count();
        push.whole_x = target.get_whole_x();
        push.dx = target.get_dx();
        push.dt = target.get_dt();
        push.window_size = vec2{ viewport_size.x, viewport_size.y };

        bind_pipeline();
        bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        bind_desc_sets({ target.get_vk_desc_sets(info.frame_index) });

        target.draw(info.command_buffer);
      }
    }
};

SELECT_COMPUTE_SHADER();
SELECT_ACTOR(game::dummy_actor);
SELECT_SHADING_SYSTEM(fdtd_wg_shading_system);

DEFINE_ENGINE(FDTD2D)
{
  public:
    ENGINE_CTOR(FDTD2D) {
      horn_ = fdtd_horn::create(
        dt,
        dx_fdtd,
        rho,
        c,
        6, // pml count
        0.5,
        { 2, 1, 2, 1, 2 }, // dimensions
        { {0.1f, 0.04f}, {0.2f, 0.03f}, {0.1f, 0.04f}, {0.1f, 0.03f}, {0.1f, 0.1f}}
      );
      horn_->build_desc(game::graphics_engine_core::get_device_r());
      add_render_target<fdtd_wg_shading_system>(*horn_);
    }

  private:
    u_ptr<fdtd_horn> horn_;
};
} // namespace hnll

int main()
{
  hnll::FDTD2D app("combination of fdtd wave-guide");
  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}