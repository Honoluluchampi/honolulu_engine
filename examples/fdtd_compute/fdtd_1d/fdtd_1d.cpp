// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>

namespace hnll {

constexpr float DX  = 3.83e-3;
constexpr float DT  = 7.81e-6;
constexpr float RHO = 1.1f;
constexpr float SOUND_SPEED = 340.f;
constexpr float V_FAC = DT / (RHO * DX);
constexpr float P_FAC = DT / RHO * SOUND_SPEED * SOUND_SPEED / DX;
constexpr uint32_t FDTD_FRAME_COUNT = 3;
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
        { set_info, set_info }, // y offset, pressure
        1
      );

      // calc initial y offsets
      std::vector<float> y_offsets(main_grid_count_);
      for (int i = 0; i < main_grid_count_; i++) {
        y_offsets[i] = calc_y(DX * i);
      }

      // initial pressure
      std::vector<float> pressures(whole_grid_count_, 0.f);

      auto y_offset_buffer = graphics::buffer::create(
        *device_,
        sizeof(float) * y_offsets.size(),
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        y_offsets.data()
      );

      auto pressure_buffer = graphics::buffer::create(
        *device_,
        sizeof(float) * pressures.size(),
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        pressures.data()
      );

      // mapped memory
      pressure_ = reinterpret_cast<float*>(pressure_buffer->get_mapped_memory());

      desc_sets_->set_buffer(0, 0, 0, std::move(y_offset_buffer));
      desc_sets_->set_buffer(1, 0, 0, std::move(pressure_buffer));
      desc_sets_->build();

      // temp
      velocity_.resize(whole_grid_count_ + 1, 0.f);
      pml_.resize(whole_grid_count_, 0.f);

      for (int i = 0; i < PML_COUNT; i++) {
        pml_[whole_grid_count_ - 1 - i] = float(PML_COUNT - i) / float(PML_COUNT);
      }
    }

    void update()
    {
      // update velocity
      velocity_[0] = 0.0007f * std::sin(40000.f * time_);
      time_ += DT;

      for (int i = 1; i < whole_grid_count_; i++) {
        velocity_[i] = (velocity_[i] - V_FAC * (pressure_[i] - pressure_[i - 1])) / (1 + pml_[i]);
      }

      // update pressure
      for (int i = 0; i < whole_grid_count_; i++) {
        pressure_[i] = (pressure_[i] - P_FAC * (velocity_[i + 1] - velocity_[i])) / (1 + pml_[i]);
      }
    }

    float get_length()     const { return length_; }
    int   get_main_grid_count() const { return main_grid_count_; }

    std::vector<VkDescriptorSet> get_frame_desc_sets()
    { return desc_sets_->get_vk_desc_sets(0); }

  private:
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    utils::single_ptr<graphics::device> device_;
    float length_ = 0.5f;

    // will be packed into vec3
    float* pressure_; // directly mapped to the desc sets
    std::vector<float> velocity_;
    std::vector<float> pml_;

    int main_grid_count_;
    int whole_grid_count_;
    float time_ = 0.f;
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
      auto vk_layout = desc_layout_->get_descriptor_set_layout();

      pipeline_layout_ = create_pipeline_layout<fdtd_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
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
      push.grid_count = field_->get_main_grid_count();
      bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

      // desc sets
      bind_desc_sets(field_->get_frame_desc_sets());

      vkCmdDraw(current_command_, 6, 1, 0, 0);

      field_->update();
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
    ENGINE_CTOR(curved_fdtd_1d) {}

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %f", 1.f / dt);
      ImGui::End();
    }
};

} // namespace hnll

int main()
{
  hnll::curved_fdtd_1d app;

  try { app.run(); }
  catch(const std::exception& e) { std::cerr << e.what() << std::endl; }
}