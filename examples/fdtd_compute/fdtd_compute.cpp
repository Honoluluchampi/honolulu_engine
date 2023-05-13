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
SELECT_ACTOR(physics::fdtd2_field);

DEFINE_ENGINE(fdtd_compute)
{
  public:
    fdtd_compute()
    {
      physics::fdtd_info info = {
        .x_len = x_len_,
        .y_len = y_len_,
        .x_impulse = x_impulse_,
        .y_impulse = y_impulse_,
        .sound_speed = sound_speed_,
        .kappa = kappa_,
        .rho = rho_,
        .f_max = f_max_
      };

      field_ = physics::fdtd2_field::create(info);
    }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %d", int(1.f/dt));
      ImGui::Text("x grid : %d", field_->get_x_grid());
      ImGui::Text("y grid : %d", field_->get_y_grid());
      ImGui::Text("dt : %f", field_->get_dt());
      ImGui::Text("grid size : %f", field_->get_grid_size());
      ImGui::Text("duration : %f", field_->get_duration());

      ImGui::SliderFloat("x length", &x_len_, 1.f, 100.f);
      ImGui::SliderFloat("y length", &y_len_, 1.f, 100.f);
      ImGui::SliderFloat("x impulse", &x_impulse_, 1.f, 100.f);
      ImGui::SliderFloat("y impulse", &y_impulse_, 1.f, 100.f);
      ImGui::SliderFloat("sound speed", &sound_speed_, 10.f, 310.f);
      ImGui::SliderFloat("kappa", &kappa_, 0.f, 10.f);
      ImGui::SliderFloat("rho", &rho_, 0.f, 0.1f);
      ImGui::SliderFloat("max freq", &f_max_, 0.f, 1000.f);

      if (ImGui::Button("restart")) {
        field_ = physics::fdtd2_field::create(
          {
            x_len_,
            y_len_,
            x_impulse_,
            y_impulse_,
            sound_speed_,
            kappa_,
            rho_,
            f_max_
          }
          );
      }
      ImGui::End();
    }

  private:
    u_ptr<physics::fdtd2_field> field_;

    float x_len_       = 10.f;
    float y_len_       = 10.f;
    float x_impulse_   = 5.f;
    float y_impulse_   = 5.f;
    float sound_speed_ = 310.f;
    float kappa_       = 10.f;
    float rho_         = 0.05f;
    float f_max_       = 120.f;
};

} // namespace hnll

int main() {
  hnll::fdtd_compute engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}