// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>

namespace hnll {

constexpr float DX  = 0.01;
constexpr float DT  = 0.01;
constexpr float RHO = 0.01;
constexpr float SOUND_SPEED = 340.f;
constexpr uint32_t FDTD_FRAME_COUNT = 3;

struct fdtd_info {
  float x_len;
  int pml_count;
  int update_per_frame;
};

std::vector<graphics::binding_info> desc_bindings = {
  { VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

float calc_y(float x)
{ return 0.015f + 0.001f * std::exp(7.5f * x); }

class fdtd_1d_field
{
  public:
    fdtd_1d_field() : device_(utils::singleton<graphics::device>::get_single_ptr())
    {
      // calc constants
      grid_count_ = static_cast<int>(length_ / DX);

      // setup desc sets
      desc_pool_ = graphics::desc_pool::builder(*device_)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10)
        .build();

      graphics::desc_set_info set_info { desc_bindings };

      desc_sets_ = graphics::desc_sets::create(
        *device_,
        desc_pool_,
        { set_info },
        1
      );

      // calc initial y offsets
      std::vector<float> y_offsets(grid_count_);
      for (int i = 0; i < grid_count_; i++) {
        y_offsets[i] = calc_y(DX * i);
      }

      auto y_offset_buffer = graphics::buffer::create(
        *device_,
        sizeof(float) * grid_count_,
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        y_offsets.data()
      );

      desc_sets_->set_buffer(0, 0, 0, std::move(y_offset_buffer));
      desc_sets_->build();
    }

    float get_length()     const { return length_; }
    int   get_grid_count() const { return grid_count_; }

    std::vector<VkDescriptorSet> get_frame_desc_sets()
    {
      return { desc_sets_->get_vk_desc_sets(0)[0] };
    }

  private:
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    utils::single_ptr<graphics::device> device_;
    float length_ = 0.5f;
    int grid_count_;
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
      auto device = utils::singleton<graphics::device>::get_single_ptr();
      desc_layout_ = graphics::desc_layout::create_from_bindings(*device, desc_bindings);

      pipeline_layout_ = create_pipeline_layout<fdtd_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        { desc_layout_->get_descriptor_set_layout() }
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

      // push constants
      fdtd_push push;
      auto window_size = game::gui_engine::get_viewport_size();
      push.window_size = { window_size.x, window_size.y };
      push.len = field_->get_length();
      push.dx  = DX;
      push.grid_count = field_->get_grid_count();
      bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

      // desc sets
      bind_desc_sets(field_->get_frame_desc_sets());

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