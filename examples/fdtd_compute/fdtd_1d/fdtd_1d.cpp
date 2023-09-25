// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>

namespace hnll {

constexpr float dx  = 0.01;
constexpr float dt  = 0.01;
constexpr float rho = 0.01;
constexpr float sound_speed = 340.f;

struct fdtd_info {
  float x_len;
  int pml_count;
  int update_per_frame;
};

class fdtd_1d_field
{
  public:
    fdtd_1d_field() = default;

  private:

};

// push constant
#include "common.h"

DEFINE_SHADING_SYSTEM(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR_DECL(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
    : field_(utils::singleton<fdtd_1d_field>::build_instance()){}

    void setup()
    {
      pipeline_layout_ = create_pipeline_layout<fdtd_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        {}
      );
      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/fdtd_compute/fdtd_1d/shaders/spv/",
        { "fdtd_1d.vert.spv", "fdtd_1d.frag.spv" },
        { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
        graphics::pipeline::default_pipeline_config_info()
      );
    }

    void render(const utils::graphics_frame_info& frame_info)
    {
      set_current_command_buffer(frame_info.command_buffer);
      bind_pipeline();

      fdtd_push push;
      auto window_size = game::gui_engine::get_viewport_size();
      push.window_size = { window_size.x, window_size.y };
      push.len = 0.5f;

      bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

      vkCmdDraw(current_command_, 6, 1, 0, 0);
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

SELECT_ACTOR(game::no_actor);
SELECT_SHADING_SYSTEM(fdtd_1d_shader);
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(curved_fdtd_1d)
{
  public:
    ENGINE_CTOR(curved_fdtd_1d)
    {}
};

} // namespace hnll

int main()
{
  hnll::curved_fdtd_1d app;

  try { app.run(); }
  catch(const std::exception& e) { std::cerr << e.what() << std::endl; }
}