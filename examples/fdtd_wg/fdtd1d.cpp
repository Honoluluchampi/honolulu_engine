// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/modules/gui_engine.hpp>
#include <game/shading_system.hpp>
#include <utils/common_alias.hpp>

// std
#include <iostream>
#include <fstream>

namespace hnll {

constexpr float dx_fdtd = 3.83e-3; // dx for fdtd grid : 3.83mm
constexpr float dt = 7.81e-6; // in seconds
const float dx_wg = dx_fdtd / std::sqrt(2); // dx for wave guide grid

constexpr float c = 340; // sound speed : 340 m/s
constexpr float rho = 1.17f;

const float v_fac = dt / (rho * dx_fdtd);
const float p_fac = dt * rho * c * c / dx_fdtd;

struct fdtd1d
{
  fdtd1d() = default;

  void build(float len, int pml_layer_count = 6)
  {
    // plus one grid for additional
    grid_count = static_cast<int>(len / dx_fdtd);
    length = len;
    pml_count = pml_layer_count;
    whole_grid_count = grid_count + pml_count * 2 + 2;
    p.resize(whole_grid_count, 0);
    v.resize(whole_grid_count, 0);
  }

  void update()
  {
    // update velocity
    for (int i = 1; i < whole_grid_count - 1; i++) {
      float pml_l = 0.5 * std::max(pml_count + 1 - i, 0) / pml_count;
      float pml_r = 0.5 * std::max(i - grid_count - pml_count, 0) / pml_count;
      float pml = std::max(pml_l, pml_r);
      v[i] -= pml * v[i] + v_fac * (p[i] - p[i - 1]);
    }
    // update pressure
    for (int i = 1; i < whole_grid_count - 1; i++) {
      float pml_l = 0.5 * std::max(pml_count + 1 - i, 0) / pml_count;
      float pml_r = 0.5 * std::max(i - grid_count - pml_count, 0) / pml_count;
      float pml = std::max(pml_l, pml_r);
      p[i] -= //pml * p[i] +
         p_fac * (v[i + 1] - v[i]);
    }
  }

  std::vector<float> get_field() const
  {
    std::vector<float> field;
    for (int i = pml_count + 1; i < whole_grid_count - (pml_count + 1); i++) {
      field.emplace_back(p[i]);
    }
    return p;
  }

  std::vector<float> p;
  std::vector<float> v;

  float ghost_v = 0;
  int grid_count; // grid count of main region
  int whole_grid_count;
  int pml_count;
  float length;
};

struct wg_section
{
  wg_section() = default;

  void build(float len)
  {
    grid_count = static_cast<int>(len / dx_wg);
    forward.resize(grid_count, 0);
    backward.resize(grid_count, 0);
    length = len;
  }

  void update(float f_input, float b_input)
  {
    f_cursor = f_cursor == 0 ? grid_count - 1 : f_cursor - 1;
    b_cursor = b_cursor + 1 == grid_count ? 0 : b_cursor + 1;

    forward[f_cursor] = f_input;
    backward[b_cursor] = b_input;
  }

  float get_first() const
  {
    if (b_cursor != grid_count - 1) {
      return backward[b_cursor + 1];
    }
    else {
      return backward[0];
    }
  }

  float get_last() const
  {
    if (f_cursor != 0) {
      return forward[f_cursor - 1];
    }
    else {
      return forward[grid_count - 1];
    }
  }

  std::vector<float> get_field() const
  {
    std::vector<float> field;
    if (grid_count == 0)
      return field;

    int f_c = f_cursor;
    int b_c = (b_cursor + 1) % grid_count;
    for (int i = 0; i < grid_count; i++) {
      field.emplace_back(forward[f_c] + backward[b_c]);
      f_c = (f_c + 1) % grid_count;
      b_c = (b_c + 1) % grid_count;
    }
    return field;
  }

  std::vector<float> forward;
  std::vector<float> backward;
  int grid_count = 0;
  float length;
  int f_cursor = 0; // indicates first element of forward
  int b_cursor = 0; //
};

std::vector<graphics::binding_info> field_bindings = {
  { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

DEFINE_PURE_ACTOR(horn)
{
  public:
    // true : fdtd-wg combined, false : fdtd only
    explicit horn(graphics::device& device, bool combined) : game::pure_actor_base<horn>()
    {
      if (combined) {
        fdtd1_.build(0.2f);
        wg_   .build(0.2f);
        fdtd_.build(0.2f);
        // impulse at 10 cm
        fdtd_.p[fdtd_.grid_count / 2] = 100.f;
      }
      else {
        fdtd1_.build(0.f);
        fdtd_.build(1.f);
      }

      // setup desc sets
      desc_pool_ = graphics::desc_pool::builder(device)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, utils::FRAMES_IN_FLIGHT)
        .build();

      graphics::desc_set_info set_info { field_bindings };
      set_info.is_frame_buffered_ = true;

      desc_sets_ = graphics::desc_sets::create(
        device,
        desc_pool_,
        { set_info },
        utils::FRAMES_IN_FLIGHT
      );

      auto initial_field = get_field();

      for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
        auto initial_buffer = graphics::buffer::create(
          device,
          sizeof(float) * initial_field.size(),
          1,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          initial_field.data()
        );

        desc_sets_->set_buffer(0, 0, i, std::move(initial_buffer));
      }
      desc_sets_->build();
    }

    // to satisfy RenderableComponent
    void bind(VkCommandBuffer command) {}
    void draw(VkCommandBuffer command) { vkCmdDraw(command, 6, 1, 0, 0); }
    uint32_t get_rc_id() const { return 0; }
    utils::shading_type get_shading_type() const { return utils::shading_type::UNIQUE; }

    // getter
    vec4 get_seg_len() const
    { return vec4{fdtd1_.length, wg_.length, fdtd_.length, 0.f }; }

    vec4 get_edge_x() const
    { return vec4{fdtd1_.length, fdtd1_.length + wg_.length, fdtd1_.length + wg_.length + fdtd_.length, 0.f }; }

    ivec4 get_idx() const
    { return ivec4{fdtd1_.grid_count, wg_.grid_count, fdtd_.grid_count, 0 }; }

    VkDescriptorSet get_vk_desc_set(int frame_index) const
    { return desc_sets_->get_vk_desc_sets(frame_index)[0]; }

    void update_this(float global_dt)
    {
      // update fdtd_only's velocity ---------------------------------------------------
      auto freq = 10000.f;
      if (frame_count++ < 15)
        fdtd_.p[fdtd_.grid_count / 2] = 100 * std::cos(frame_count * dt * freq * M_PI * 2);

      fdtd_.update();

      ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      if (ImGui::Button("impulse")) {
        frame_count = 0;
      }
      ImGui::End();
    }

    std::vector<float> get_field() const
    {
      std::vector<float> field;
      for (const auto& v : fdtd_.get_field()) {
        field.emplace_back(v);
      }
      return field;
    }

    void update_field(int frame_index)
    {
      auto data = get_field();
      desc_sets_->write_to_buffer(0, 0, frame_index, data.data());
      desc_sets_->flush_buffer(0, 0, frame_index);
    }

  private:
    fdtd1d fdtd1_;
    wg_section   wg_;
    fdtd1d fdtd_;

    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;

    int frame_count = 0;
};

// shared with shaders
#include "common.h"

DEFINE_SHADING_SYSTEM(fdtd_wg_shading_system, horn)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(fdtd_wg_shading_system, horn);

    void setup()
    {
      shading_type_ = utils::shading_type::UNIQUE;

      desc_layout_ = graphics::desc_layout::create_from_bindings(device_, field_bindings);

      pipeline_layout_ = create_pipeline_layout<fdtd_wg_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        { desc_layout_->get_descriptor_set_layout() }
      );

      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/fdtd_wg/shaders/spv/",
        { "fdtd_wg.vert.spv", "fdtd_wg.frag.spv" },
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
        set_current_command_buffer(info.command_buffer);

        auto viewport_size = game::gui_engine::get_viewport_size();
        fdtd_wg_push push;
        push.h_width = 30;
        push.w_width = viewport_size.x;
        push.w_height = viewport_size.y;
        push.seg_len = target.get_seg_len();
        push.edge_x = target.get_edge_x();
        push.idx = target.get_idx();

        // update field
        target.update_this(1);
        target.update_field(info.frame_index);

        bind_pipeline();
        bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        bind_desc_sets({ target.get_vk_desc_set(info.frame_index) });

        target.draw(info.command_buffer);
      }
    }
};

SELECT_COMPUTE_SHADER();
SELECT_ACTOR(game::dummy_actor);
SELECT_SHADING_SYSTEM(fdtd_wg_shading_system);

DEFINE_ENGINE(FDTD_WG)
{
  public:
    ENGINE_CTOR(FDTD_WG) {
      horn_ = std::make_unique<horn>(game::graphics_engine_core::get_device_r(), false);
      add_render_target<fdtd_wg_shading_system>(*horn_);
    }

  private:
    u_ptr<horn> horn_;
};
} // namespace hnll

int main()
{
  hnll::FDTD_WG app("combination of fdtd wave-guide");
  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}