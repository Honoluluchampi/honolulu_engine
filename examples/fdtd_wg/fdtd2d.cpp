// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/modules/gui_engine.hpp>
#include <game/shading_system.hpp>
#include <utils/common_alias.hpp>

#include "fdtd_horn.hpp"

// std
#include <iostream>

// lib
#include <AudioFile/AudioFile.h>

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

  void build(
    float w,
    float h,
    int pml_layer_count = 6,
    bool up_pml = true,
    bool down_pml = true,
    bool left_pml = true,
    bool right_pml = true)
  {
    // grid_count of main domain of each axis
    main_grid_count.x() = static_cast<int>(w / dx_fdtd);
    main_grid_count.y() = static_cast<int>(h / dx_fdtd);
    height = h;
    width = w;
    pml_count = pml_layer_count;
    grid_count.x() = main_grid_count.x() + pml_count * left_pml + pml_count * right_pml + 2;
    grid_count.y() = main_grid_count.y() + pml_count * up_pml + pml_count * down_pml + 2;
    whole_grid_count = grid_count.x() * grid_count.y();
    p.resize(whole_grid_count, 0);
    vx.resize(whole_grid_count, 0);
    vy.resize(whole_grid_count, 0);
    pml_up = up_pml;
    pml_down = down_pml;
    pml_left = left_pml;
    pml_right = right_pml;
  }

  void update()
  {
    float pml_max = 0.5;
    // update velocity
    for (int j = 1; j < grid_count.y() - 1; j++) {
      for (int i = 1; i < grid_count.x() - 1; i++) {
        if (j == 1 && i < grid_count.x() * 4 / 5)
          continue;
        float pml_l = pml_left * pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        float pml_r = pml_right * pml_max * std::max(i - main_grid_count.x() - pml_count * pml_left, 0) / pml_count;
        float pml_x = std::max(pml_l, pml_r);
        float pml_u = pml_up * pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        float pml_d = pml_down * pml_max * std::max(j - main_grid_count.y() - pml_count * pml_up, 0) / pml_count;
        float pml_y = std::max(pml_u, pml_d);
        float pml = std::max(pml_x, pml_y);

        vx[ELEM_2D(i, j)] = (vx[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i - 1, j)])) / (1 + pml);
        vy[ELEM_2D(i, j)] = (vy[ELEM_2D(i, j)] - v_fac * (p[ELEM_2D(i, j)] - p[ELEM_2D(i, j - 1)])) / (1 + pml);
      }
    }
    // update pressure
    for (int j = 1; j < grid_count.y() - 1; j++) {
      for (int i = 1; i < grid_count.x() - 1; i++) {
        float pml_l = pml_left * pml_max * std::max(pml_count + 1 - i, 0) / pml_count;
        float pml_r = pml_right * pml_max * std::max(i - main_grid_count.x() - pml_count * pml_left, 0) / pml_count;
        float pml_x = std::max(pml_l, pml_r);
        float pml_u = pml_up * pml_max * std::max(pml_count + 1 - j, 0) / pml_count;
        float pml_d = pml_down * pml_max * std::max(j - main_grid_count.y() - pml_count * pml_up, 0) / pml_count;
        float pml_y = std::max(pml_u, pml_d);
        float pml = std::max(pml_x, pml_y);

//        if (i <= grid_count.x() / 3) {
          p[ELEM_2D(i, j)] = (p[ELEM_2D(i, j)] - p_fac *
            (vx[ELEM_2D(i + 1, j)] - vx[ELEM_2D(i, j)] + vy[ELEM_2D(i, j + 1)] - vy[ELEM_2D(i, j)]))
               / (1 + pml);
//        }
//        else if (i <= grid_count.x() * 2 / 3){
//          if (j == 1) {
//            p[ELEM_2D(i, j)] = (p[ELEM_2D(i, j)] - p_fac * (vx[ELEM_2D(i + 1, grid_count.y() / 2)] - vx[ELEM_2D(i, grid_count.y() / 2)])) / (1 + pml_x);
//          }
//          else {
//            p[ELEM_2D(i, j)] = p[ELEM_2D(i, 1)];
//          }
//        }
//        else {
//          p[ELEM_2D(i, j)] = (p[ELEM_2D(i, j)] - p_fac *
//                                                 (vx[ELEM_2D(i + 1, j)] - vx[ELEM_2D(i, j)] + vy[ELEM_2D(i, j + 1)] - vy[ELEM_2D(i, j)]))
//                             / (1 + pml);
//        }
      }
    }
  }

  std::vector<float> get_field() const
  {
    std::vector<float> field;
    for (int j = pml_count * pml_up + 1; j < grid_count.y() - (pml_count * pml_down + 1); j++) {
      for (int i = pml_count * pml_left + 1; i < grid_count.x() - (pml_count * pml_right + 1); i++) {
        field.emplace_back(p[ELEM_2D(i, j)]);
      }
    }
    return field;
  }

  std::vector<float> p;
  std::vector<float> vx;
  std::vector<float> vy;

  ivec2 main_grid_count; // grid count of main region
  ivec2 grid_count;
  int whole_grid_count;
  int pml_count;
  float height;
  float width;
  bool pml_up;
  bool pml_down;
  bool pml_left;
  bool pml_right;
};

struct fdtd12_push {
  int segment_count; // element count of segment info
  float horn_x_max;
  int pml_count;
  int whole_grid_count;
  float dx;
  float dt;
  vec2 window_size;
};

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
        "/examples/fdtd_wg/shaders/spv/",
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
        push.segment_count = target.get_segment_count();
        push.window_size = vec2{ viewport_size.x, viewport_size.y };
        push.horn_x_max = target.get_x_max();
        push.pml_count = target.get_pml_count();
        push.whole_grid_count = target.get_whole_grid_count();
        push.dx = target.get_dx();
        push.dt = target.get_dt();

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
        2, // pml count
        { 2, 1, 2, 1, 2 }, // dimensions
        { {0.1f, 0.04f}, {0.2f, 0.03f}, {0.1f, 0.04f}, {0.1f, 0.03f}, {0.07f, 0.07f}}
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