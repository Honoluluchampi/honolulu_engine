// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/compute_shader.hpp>

namespace hnll {

constexpr float DX  = 3.83e-3;
constexpr float DT  = 7.81e-6;
constexpr float RHO = 1.1f;
constexpr float SOUND_SPEED = 340.f;
constexpr float V_FAC = DT / (RHO * DX);
constexpr float P_FAC = DT / RHO * SOUND_SPEED * SOUND_SPEED / DX;
constexpr uint32_t FDTD_FRAME_COUNT = 2;
constexpr uint32_t PML_COUNT = 6;

struct fdtd_info {
  float x_len;
  int pml_count;
  int update_per_frame;
};

std::vector<graphics::binding_info> desc_bindings = {
  { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

float calc_y(float x)
{ return 0.015f + 0.001f * std::exp(7.5f * x); }

// push constant, particle
#include "common.h"

class fdtd_1d_field
{
  public:
    fdtd_1d_field() : device_(utils::singleton<graphics::device>::get_single_ptr())
    {
      // calc constants
      main_grid_count_ = static_cast<int>(length_ / DX);
      whole_grid_count_ = PML_COUNT + main_grid_count_;

      // setup desc sets
      desc_pool_ = graphics::desc_pool::builder(*device_)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10)
        .build();

      graphics::desc_set_info set_info { desc_bindings };

      desc_sets_ = graphics::desc_sets::create(
        *device_,
        desc_pool_,
        { set_info, set_info }, // curr, prev
        1
      );

      // initial pressure
      std::vector<particle> particles(whole_grid_count_ + 1, particle(0.f, 0.f, 0.f, 0.f));
      // calc initial y offsets
      for (int i = 0; i < whole_grid_count_; i++) {
        particles[i].y_offset = calc_y(DX * i);
      }
      // set pml
      for (int i = 0; i < PML_COUNT; i++) {
        particles[whole_grid_count_ - 1 - i].pml = float(PML_COUNT - i) / float(PML_COUNT);
      }

      auto curr_buffer = graphics::buffer::create(
        *device_,
        sizeof(particle) * particles.size(),
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        particles.data()
      );

      auto prev_buffer = graphics::buffer::create(
        *device_,
        sizeof(particle) * particles.size(),
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        particles.data()
      );

      // mapped memory
      buffer0_ = reinterpret_cast<particle*>(curr_buffer->get_mapped_memory());
      buffer1_ = reinterpret_cast<particle*>(prev_buffer->get_mapped_memory());

      desc_sets_->set_buffer(0, 0, 0, std::move(curr_buffer));
      desc_sets_->set_buffer(1, 0, 0, std::move(prev_buffer));
      desc_sets_->build();
    }

    void update()
    {
      duration_ += DT;
      frame_index = frame_index == FDTD_FRAME_COUNT - 1 ? 0 : frame_index + 1;
    }

    float get_length()           const { return length_; }
    float get_duration()         const { return duration_; }
    int   get_main_grid_count()  const { return main_grid_count_; }
    int   get_whole_grid_count() const { return whole_grid_count_; }
    float get_current_p() const { return buffer0_[0].p; }

    std::vector<VkDescriptorSet> get_frame_desc_sets()
    {
      auto sets = desc_sets_->get_vk_desc_sets(0);
      if (frame_index == 0)
        return { sets[0], sets[1] };
      else
        return { sets[1], sets[0] };
    }

  private:
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    utils::single_ptr<graphics::device> device_;
    float length_ = 0.5f;

    particle* buffer0_;
    particle* buffer1_;

    int main_grid_count_;
    int whole_grid_count_;
    float duration_ = 0.f;

    int frame_index = 0;
};

DEFINE_SHADING_SYSTEM(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR_DECL(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
    : field_(utils::singleton<fdtd_1d_field>::build_instance()){}

    void setup()
    {
      auto device = utils::singleton<graphics::device>::get_single_ptr();
      desc_layout_ = graphics::desc_layout::create_from_bindings(*device, desc_bindings);
      auto vk_layout = desc_layout_->get_descriptor_set_layout();

      pipeline_layout_ = create_pipeline_layout<fdtd_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT),
        { vk_layout, vk_layout }
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
      push.dt  = DT;
      push.duration = field_->get_duration();
      push.main_grid_count = field_->get_main_grid_count();
      push.whole_grid_count = field_->get_whole_grid_count();
      push.v_fac = V_FAC;
      push.p_fac = P_FAC;
      bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

      // desc sets
      bind_desc_sets(field_->get_frame_desc_sets());

      vkCmdDraw(current_command_, 6, 1, 0, 0);

      field_->update();
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

DEFINE_COMPUTE_SHADER(fdtd_1d_compute)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(fdtd_1d_compute) : field_(utils::singleton<fdtd_1d_field>::get_single_ptr()) {}

    void setup()
    {
      desc_layout_ = graphics::desc_layout::create_from_bindings(*device_, desc_bindings);
      auto vk_layout = desc_layout_->get_descriptor_set_layout();

      pipeline_ = create_pipeline<fdtd_push>(
        utils::get_engine_root_path() + "/examples/fdtd_compute/fdtd_1d/shaders/spv/fdtd_1d.comp.spv",
        { vk_layout, vk_layout }
      );
    }

    void render(const utils::compute_frame_info& info)
    {
      auto command = info.command_buffer;
      bind_pipeline(command);

      // push constants
      fdtd_push push;
      auto window_size = game::gui_engine::get_viewport_size();
      push.window_size = { window_size.x, window_size.y };
      push.len = field_->get_length();
      push.dx  = DX;
      push.dt  = DT;
      push.duration = field_->get_duration();
      push.main_grid_count = field_->get_main_grid_count();
      push.whole_grid_count = field_->get_whole_grid_count();
      push.v_fac = V_FAC;
      push.p_fac = P_FAC;
      bind_push(
        command,
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_COMPUTE_BIT),
        push
      );

      // desc sets
      bind_desc_sets(command, field_->get_frame_desc_sets());
      dispatch_command(
        command,
        (int(field_->get_whole_grid_count()) + fdtd1_local_size_x - 1) / fdtd1_local_size_x,
        1,
        1
      );
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

SELECT_ACTOR(game::no_actor);
SELECT_SHADING_SYSTEM(fdtd_1d_shader);
SELECT_COMPUTE_SHADER(fdtd_1d_compute);

DEFINE_ENGINE(curved_fdtd_1d)
{
  public:
    explicit ENGINE_CTOR(curved_fdtd_1d), field_(utils::singleton<fdtd_1d_field>::build_instance())
    {
      set_max_fps(1000.f);
    }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %f", 1.f / dt);
      ImGui::Text("duration : %f", field_->get_duration());
      ImGui::Text("p : %f", field_->get_current_p());
      ImGui::End();
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

} // namespace hnll

int main()
{
  hnll::utils::vulkan_config config;
  config.present = hnll::utils::present_mode::IMMEDIATE;
  hnll::curved_fdtd_1d app("1d fdtd", config);

  try { app.run(); }
  catch(const std::exception& e) { std::cerr << e.what() << std::endl; }
}