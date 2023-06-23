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

const float segment_length = 0.2; // 20 cm

struct fdtd_section
{
  fdtd_section() = default;

  void build(float len)
  {
    // plus one grid for additional
    grid_count = static_cast<int>(len / dx_fdtd);
    p.resize(grid_count, 0);
    v.resize(grid_count, 0);
    length = len;
  }

  std::vector<float> p;
  std::vector<float> v;
  float ghost_v = 0;
  int grid_count;
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
      field.emplace_back(forward[f_cursor] + backward[b_cursor]);
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
        fdtd2_.build(0.2f);
        // impulse at 10 cm
        fdtd2_.p[fdtd2_.grid_count / 2] = 500.f;
      }
      else {
        fdtd2_.build(0.6f);
        // impulse at 10 cm
        fdtd2_.p[fdtd2_.grid_count * 5 / 6] = 500.f;
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
        auto initial_buffer = graphics::buffer::create_with_staging(
          device,
          sizeof(float) * initial_field.size(),
          1,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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
    vec3 get_seg_len() const
    { return vec3{ fdtd1_.length, wg_.length, fdtd2_.length }; }

    vec3 get_edge_x() const
    { return vec3{ fdtd1_.length, fdtd1_.length + wg_.length, fdtd1_.length + wg_.length + fdtd2_.length }; }

    ivec3 get_idx() const
    { return ivec3{ fdtd1_.grid_count, wg_.grid_count, fdtd2_.grid_count }; }

    VkDescriptorSet get_vk_desc_set() const
    { return desc_sets_->get_vk_desc_sets(0)[0]; }

    void update_this(float global_dt)
    {
      // update fdtd_only's velocity ---------------------------------------------------
      for (int i = 1; i < fdtd2_.grid_count - 1; i++) {
        fdtd2_.v[i] -= v_fac * (fdtd2_.p[i] - fdtd2_.p[i - 1]);
      }
      // update fdtd_only's pressure ---------------------------------------------------
      for (int i = 0; i < fdtd1_.grid_count - 1; i++) {
        fdtd1_.p[i] -= p_fac * (fdtd1_.v[i + 1] - fdtd1_.v[i]);
      }

      // update combined model velocity ------------------------------------------------
      // left part
      for (int i = 1; i < fdtd1_.grid_count; i++) {
        fdtd1_.v[i] -= v_fac * (fdtd1_.p[i] - fdtd1_.p[i - 1]);
      }
      // junction part
      fdtd1_.ghost_v -= v_fac * (wg_.get_first() - fdtd1_.p[fdtd1_.grid_count - 1]);

      // right part
      for (int i = 1; i < fdtd2_.grid_count; i++) {
        fdtd2_.v[i] -= v_fac * (fdtd2_.p[i] - fdtd2_.p[i - 1]);
      }
      // junction
      fdtd2_.v[0] -= v_fac * (fdtd2_.p[0] - wg_.get_last());

      // update combined model pressure -----------------------------------------------
      // left
      for (int i = 0; i < fdtd1_.grid_count - 1; i++) {
        fdtd1_.p[i] -= p_fac * (fdtd1_.v[i + 1] - fdtd1_.v[i]);
      }
      // junction
      fdtd1_.p[fdtd1_.grid_count - 1] -= p_fac * (fdtd1_.ghost_v - fdtd1_.v[fdtd1_.grid_count - 1]);

      // right (include junction)
      for (int i = 0; i < fdtd2_.grid_count - 1; i++) {
        fdtd2_.p[i] -= p_fac * (fdtd2_.v[i + 1] - fdtd2_.v[i]);
      }

      // wave guide
      wg_.update(fdtd2_.p[fdtd2_.grid_count - 1], fdtd2_.p[0]);
    }

    std::vector<float> get_field() const
    {
      std::vector<float> field;
      for (const auto& v : fdtd1_.p) {
        field.emplace_back(v);
      }
      auto wg_field = wg_.get_field();
      for (const auto& v : wg_field) {
        field.emplace_back(v);
      }
      for (const auto& v : fdtd2_.p) {
        field.emplace_back(v);
      }
      return field;
    }

  private:
    fdtd_section fdtd1_;
    wg_section   wg_;
    fdtd_section fdtd2_;

    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
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
        push.h_width = 50;
        push.w_width = viewport_size.x;
        push.w_height = viewport_size.y;
        push.seg_len = target.get_seg_len();
        push.edge_x = target.get_edge_x();
        push.idx = target.get_idx();

        bind_pipeline();
        bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        bind_desc_sets({ target.get_vk_desc_set() });

        target.draw(info.command_buffer);
      }
    }
};

SELECT_COMPUTE_SHADER();
SELECT_ACTOR();
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