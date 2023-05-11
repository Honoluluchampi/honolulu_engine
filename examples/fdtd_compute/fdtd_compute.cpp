// hnll
#include <game/engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// lib
#include <imgui/imgui.h>

namespace hnll {

SELECT_SHADING_SYSTEM(physics::fdtd2_shading_system);
SELECT_COMPUTE_SHADER(physics::fdtd2_compute_shader);
SELECT_ACTOR();

DEFINE_ENGINE(fdtd_compute)
{
  public:
    fdtd_compute()
    {
      physics::fdtd_info info = {
        .x_len = 10.f,
        .y_len = 10.f,
        .sound_speed = 10.f,
        .kappa = 0.1f,
        .rho = 0.001f,
        .f_max = 100.f
      };

      field_ = physics::fdtd2_field::create(info);
    }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %d", int(1.f/dt));
      ImGui::Text("x grid : %d", field_->get_x_grid());
      ImGui::Text("y grid : %d", field_->get_y_grid());
      ImGui::Text("duration : %f", field_->get_duration());
      if (ImGui::Button("restart")) {
        field_->set_restart(2);
        field_->reset_duration();
      }
      ImGui::End();
    }

  private:
    u_ptr<physics::fdtd2_field> field_;
};

} // namespace hnll

int main() {
  hnll::fdtd_compute engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}