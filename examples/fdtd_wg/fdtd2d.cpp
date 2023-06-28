// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/modules/gui_engine.hpp>
#include <game/shading_system.hpp>
#include <utils/common_alias.hpp>

// std
#include <iostream>
#include <fstream>

#define ELEM_2D(i, j) (i) + grid_count.x() * (j)

namespace hnll {

constexpr float dx_fdtd = 3.83e-3; // dx for fdtd grid : 3.83mm
constexpr float dt = 7.81e-6; // in seconds

constexpr float c = 340; // sound speed : 340 m/s
constexpr float rho = 1.17f;

constexpr float v_fac = dt / (rho * dx_fdtd);
constexpr float p_fac = dt * rho * c * c / dx_fdtd;

struct fdtd2d
{
  fdtd2d() = default;

  void build(float w, float h, int pml_layer_count = 20)
  {
    // grid_count of main domain of each axis
    main_grid_count.x() = static_cast<int>(w / dx_fdtd);
    main_grid_count.y() = static_cast<int>(h / dx_fdtd);
    height = h;
    width = w;
    pml_count = pml_layer_count;
    grid_count.x() = main_grid_count.x() + pml_count * 2 + 2;
    grid_count.y() = main_grid_count.y() + pml_count * 2 + 2;
    whole_grid_count = grid_count.x() * grid_count.y();
    p.resize(whole_grid_count, 0);
    vx.resize(whole_grid_count, 0);
    vy.resize(whole_grid_count, 0);
  }

  void update()
  {
    float pml_max = 0.5;
    // update velocity
    for (int j = 1; j < grid_count.y() - 1; j++) {
      for (int i = 1; i < grid_count.x() - 1; i++) {
        // x velocity
        float pml_l = pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        float pml_r = pml_max * std::max(i - main_grid_count.x() - pml_count, 0) / pml_count;
        float pml_x = std::max(pml_l, pml_r);

        // y velocity
        float pml_u = pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        float pml_d = pml_max * std::max(j - main_grid_count.y() - pml_count, 0) / pml_count;
        float pml_y = std::max(pml_u, pml_d);

        float pml = std::max(pml_x, pml_y);

        vx[ELEM_2D(i, j)] = (vx[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i - 1, j)])) / (1 + pml);
        vy[ELEM_2D(i, j)] = (vy[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i, j - 1)])) / (1 + pml);
      }
    }
    // update pressure
    for (int j = 1; j < grid_count.y() - 1; j++) {
      for (int i = 1; i < grid_count.x() - 1; i++) {
        float pml_l = pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        float pml_r = pml_max * std::max(i - main_grid_count.x() - pml_count, 0) / pml_count;
        float pml_x = std::max(pml_l, pml_r);
        float pml_u = pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        float pml_d = pml_max * std::max(j - main_grid_count.y() - pml_count, 0) / pml_count;
        float pml_y = std::max(pml_u, pml_d);
        float pml = std::max(pml_x, pml_y);
        p[ELEM_2D(i, j)] = (p[ELEM_2D(i, j)] -
          p_fac * (vx[ELEM_2D(i + 1, j)] - vx[ELEM_2D(i, j)] + vy[ELEM_2D(i, j + 1)] - vy[ELEM_2D(i, j)]))
            / (1 + pml);
      }
    }
  }

  std::vector<float> get_field() const
  {
    std::vector<float> field;
    for (int j = pml_count + 1; j < grid_count.y() - (pml_count + 1); j++) {
      for (int i = pml_count + 1; i < grid_count.x() - (pml_count + 1); i++) {
        field.emplace_back(p[ELEM_2D(i, j)]);
      }
    }
    return field;
  }

  std::vector<float> p;
  std::vector<float> vx;
  std::vector<float> vy;

  float ghost_v = 0;
  ivec2 main_grid_count; // grid count of main region
  ivec2 grid_count;
  int whole_grid_count;
  int pml_count;
  float height;
  float width;
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
      fdtd_.build(0.4f, 0.033f);

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
    vec2 get_dim() const  { return { fdtd_.width, fdtd_.height };}
    ivec2 get_main_grid_count() const { return fdtd_.main_grid_count; }

    VkDescriptorSet get_vk_desc_set(int frame_index) const
    { return desc_sets_->get_vk_desc_sets(frame_index)[0]; }

    void update_this(float global_dt)
    {
      // update fdtd_only's velocity ---------------------------------------------------
      auto freq = 4000.f;
      if (frame_count++ < 15) {
        auto grid = fdtd_.grid_count;
        auto idx = grid.x() / 6 + grid.x() * grid.y() / 2;
        fdtd_.p[idx] = 100 * std::cos(frame_count * dt * freq * M_PI * 2);
      }
      fdtd_.update();

      ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      if (ImGui::Button("impulse")) {
        frame_count = 0;
      }
      ImGui::End();
    }

    std::vector<float> get_field() const
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
        push.h_dim = target.get_dim();
        push.w_dim = {viewport_size.x, viewport_size.y};
        push.grid_count = target.get_main_grid_count();

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