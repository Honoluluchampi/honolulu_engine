// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/modules/gui_engine.hpp>
#include <game/shading_system.hpp>
#include <utils/common_alias.hpp>

// std
#include <iostream>
#include <fstream>

#define ELEM_2D(i, j) (i) + grid_per_axis * (j)

namespace hnll {

constexpr double dx_fdtd = 3.83e-3; // dx for fdtd grid : 3.83mm
constexpr double dt = 7.81e-6; // in seconds

constexpr double c = 340; // sound speed : 340 m/s
constexpr double rho = 1.17f;

constexpr double v_fac = dt / (rho * dx_fdtd);
constexpr double p_fac = dt * rho * c * c / dx_fdtd;

struct fdtd2d
{
  fdtd2d() = default;

  void build(double len, int pml_layer_count = 6)
  {
    // grid_count of main domain of each axis
    grid_count = static_cast<int>(len / dx_fdtd);
    length = len;
    pml_count = pml_layer_count;
    grid_per_axis = grid_count + pml_count * 2 + 2;
    whole_grid_count = grid_per_axis * grid_per_axis;
    p.resize(whole_grid_count, 0);
    vx.resize(whole_grid_count, 0);
    vy.resize(whole_grid_count, 0);
  }

  void update()
  {
    double pml_max = 0.5;
    // update velocity
    for (int j = 1; j < grid_per_axis - 1; j++) {
      for (int i = 1; i < grid_per_axis - 1; i++) {
        // x velocity
        double pml_l = pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        double pml_r = pml_max * std::max(i - grid_count - pml_count, 0) / pml_count;
        double pml_x = std::max(pml_l, pml_r);

        // y velocity
        double pml_u = pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        double pml_d = pml_max * std::max(j - grid_count - pml_count, 0) / pml_count;
        double pml_y = std::max(pml_u, pml_d);

        double pml = std::max(pml_x, pml_y);

        vx[ELEM_2D(i, j)] = (vx[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i - 1, j)])) / (1 + pml);
        vy[ELEM_2D(i, j)] = (vy[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i, j - 1)])) / (1 + pml);
      }
    }
    // update pressure
    for (int j = 1; j < grid_per_axis - 1; j++) {
      for (int i = 1; i < grid_per_axis - 1; i++) {
        double pml_l = pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        double pml_r = pml_max * std::max(i - grid_count - pml_count, 0) / pml_count;
        double pml_x = std::max(pml_l, pml_r);
        double pml_u = pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        double pml_d = pml_max * std::max(j - grid_count - pml_count, 0) / pml_count;
        double pml_y = std::max(pml_u, pml_d);
        double pml = std::max(pml_x, pml_y);
        p[ELEM_2D(i, j)] = (p[ELEM_2D(i, j)] -
          p_fac * (vx[ELEM_2D(i + 1, j)] - vx[ELEM_2D(i, j)] + vy[ELEM_2D(i, j + 1)] - vy[ELEM_2D(i, j)]))
            / (1 + pml);
      }
    }
  }

  std::vector<double> get_field() const
  {
    std::vector<double> field;
    for (int i = pml_count + 1; i < grid_per_axis - (pml_count + 1); i++) {
      for (int j = pml_count + 1; j < grid_per_axis - (pml_count + 1); j++) {
        field.emplace_back(p[ELEM_2D(i, j)]);
      }
    }
    return field;
  }

  std::vector<double> p;
  std::vector<double> vx;
  std::vector<double> vy;

  float ghost_v = 0;
  int grid_count; // grid count of main region
  int grid_per_axis;
  int whole_grid_count;
  int pml_count;
  double length;
};

std::vector<graphics::binding_info> field_bindings = {
  { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

DEFINE_PURE_ACTOR(horn)
{
  public:
    // true : fdtd-wg combined, false : fdtd only
    explicit horn(graphics::device& device) : game::pure_actor_base<horn>()
    {
      fdtd_.build(0.2f);

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
          sizeof(double) * initial_field.size(),
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
    double get_length() const { return fdtd_.length; }
    int get_grid_count() const { return fdtd_.grid_count; }

    VkDescriptorSet get_vk_desc_set(int frame_index) const
    { return desc_sets_->get_vk_desc_sets(frame_index)[0]; }

    void update_this(float global_dt)
    {
      // update fdtd_only's velocity ---------------------------------------------------
      auto freq = 10000.f;
      if (frame_count++ < 15) {
        auto half_idx = fdtd_.grid_per_axis / 2;
        auto idx = half_idx + fdtd_.grid_per_axis * half_idx;
        fdtd_.p[idx] = 100 * std::cos(frame_count * dt * freq * M_PI * 2);
      }
      fdtd_.update();

      ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      if (ImGui::Button("impulse")) {
        frame_count = 0;
      }
      ImGui::End();
    }

    std::vector<double> get_field() const
    {
      return fdtd_.get_field();
    }

    void update_field(int frame_index)
    {
      auto data = get_field();
      desc_sets_->write_to_buffer(0, 0, frame_index, data.data());
      desc_sets_->flush_buffer(0, 0, frame_index);
    }

  private:
    fdtd2d fdtd_;

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

      pipeline_layout_ = create_pipeline_layout<fdtd_2d_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        { desc_layout_->get_descriptor_set_layout() }
      );

      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/fdtd_wg/shaders/spv/",
        { "fdtd_wg.vert.spv", "fdtd2d.frag.spv" },
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
        fdtd_2d_push push;
        push.h_len = target.get_length();
        push.w_width = viewport_size.x;
        push.w_height = viewport_size.y;
        push.grid_count = target.get_grid_count();

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

DEFINE_ENGINE(FDTD2D)
{
  public:
    ENGINE_CTOR(FDTD2D) {
      horn_ = std::make_unique<horn>(game::graphics_engine_core::get_device_r());
      add_render_target<fdtd_wg_shading_system>(*horn_);
    }

  private:
    u_ptr<horn> horn_;
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