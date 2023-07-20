// hnll
#include <game/engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// std
#include <thread>

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
        .sound_speed = sound_speed_,
        .rho = rho_,
        .pml_count = 6,
        .update_per_frame = 2134
      };

      field_ = physics::fdtd2_field::create(info);
      field_->set_as_target(field_.get());
    }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %d", int(1.f/dt));
      ImGui::Text("x grid : %d", field_->get_x_grid());
      ImGui::Text("y grid : %d", field_->get_y_grid());
      ImGui::Text("dt : %f", field_->get_dt());
      ImGui::Text("grid size : %f", field_->get_dx());
      ImGui::Text("duration : %f", field_->get_duration());

      ImGui::SliderFloat("x length", &x_len_, 0.1f, 0.8f);
      ImGui::SliderFloat("y length", &y_len_, 0.1f, 0.4f);
      ImGui::SliderFloat("sound speed", &sound_speed_, 10.f, 340.f);
      ImGui::SliderFloat("rho", &rho_, 1.f, 2.f);
      ImGui::SliderInt("update per frame : %d", &update_per_frame_, 2, 2134);

      field_->add_duration();
      field_->set_update_per_frame(update_per_frame_);

      if (ImGui::Button("restart")) {
        std::thread t([this] {
          this->staging_field_ = physics::fdtd2_field::create(
            {
              this->x_len_,
              this->y_len_,
              this->sound_speed_,
              this->rho_,
              6,
              update_per_frame_
            }
          );
        });
        t.detach();
        wait_for_construction_ = true;
      }
      ImGui::End();

      if (wait_for_construction_) {
        if (staging_field_ != nullptr && staging_field_->is_ready()) {
          auto tmp = std::move(field_);
          field_ = std::move(staging_field_);
          staging_field_ = std::move(tmp);
          field_->set_as_target(field_.get());
          wait_for_construction_ = false;
        }
      }
    }

  private:
    u_ptr<physics::fdtd2_field> field_;
    u_ptr<physics::fdtd2_field> staging_field_;
    bool wait_for_construction_ = false;

    float x_len_       = 0.6f;
    float y_len_       = 0.3f;
    float sound_speed_ = 340.f;
    float rho_         = 1.1f;
    int   update_per_frame_ = 2134;
};

} // namespace hnll

int main() {
  hnll::fdtd_compute engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}