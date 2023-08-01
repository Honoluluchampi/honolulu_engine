// hnll
#include <game/engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>
#include "serial.hpp"

// std
#include <thread>

// lib
#include <imgui/imgui.h>

#define AUDIO_FRAME_RATE 128000
#define DEFAULT_UPDATE_PER_FRAME 4268

#define ARDUINO "/dev/ttyACM0"

namespace hnll {

SELECT_SHADING_SYSTEM(physics::fdtd2_shading_system);
SELECT_COMPUTE_SHADER(physics::fdtd2_compute_shader);
SELECT_ACTOR(physics::fdtd2_field);

DEFINE_ENGINE(fdtd_compute)
{
  public:
    fdtd_compute() : serial_(ARDUINO)
    {
      set_max_fps(30.f);
      auto game_fps = get_max_fps();

      update_per_frame_ = std::ceil(float(AUDIO_FRAME_RATE) / game_fps);
      if (update_per_frame_ % 2 != 0)
        update_per_frame_ += 1;

      physics::fdtd_info info = {
        .x_len = x_len_,
        .y_len = y_len_,
        .sound_speed = sound_speed_,
        .rho = rho_,
        .pml_count = 6,
        .update_per_frame = update_per_frame_
      };

      field_ = physics::fdtd2_field::create(info);
      field_->set_as_target(field_.get());

      audio::engine::start_hae_context();
      source_ = audio::engine::get_available_source_id();

      // get gui input
      game::engine_core::add_glfw_mouse_button_callback([this](GLFWwindow* win, int button, int action, int mods){
        this->process_input(button, action);
      });
    }

    void cleanup() { field_.reset(); audio::engine::kill_hae_context(); }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("graphics fps : %d", int(1.f/dt));
      ImGui::Text("x grid : %d", field_->get_x_grid());
      ImGui::Text("y grid : %d", field_->get_y_grid());
      ImGui::Text("dt : %f", field_->get_dt());
      ImGui::Text("grid size : %f", field_->get_dx());
      ImGui::Text("duration : %f", field_->get_duration());
      ImGui::Text("cursor x : %f", cursor_pos_.x());
      ImGui::Text("cursor y : %f", cursor_pos_.y());

      ImGui::SliderFloat("x length", &x_len_, 0.1f, 0.8f);
      ImGui::SliderFloat("y length", &y_len_, 0.1f, 0.4f);
      ImGui::SliderInt("update per frame : %d", &update_per_frame_, 3, 4268);
      ImGui::SliderFloat("input pressure : %f", &mouth_pressure_, 0.f, 5000.f);
      ImGui::SliderFloat("amplify : %f", &amplify_, 0.f, 300.f);

      // get input from arduino
      auto data = serial_.read_data('/');
      if (data[0] == '/' && data[4] == '/') {
        int raw = atoi(data.substr(1, 3).c_str());
        float a = 0.7f;
        mouth_pressure_ = a * std::max((raw - 70) * 500, 0) + (1 - a) * mouth_pressure_;
      }

      field_->add_duration();
      field_->set_update_per_frame(update_per_frame_);
      field_->set_mouth_pressure(mouth_pressure_);

      update_gui();
      update_sound();

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

      if (ImGui::Button("open/close hole")) {
        field_->set_tone_hole_state(!field_->get_tone_hole_is_open());
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

    void process_input(int button, int action)
    {
      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        button_pressed_ = true;
      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        button_pressed_ = false;
    }

    void update_gui()
    {
      if (button_pressed_) {
        cursor_pos_ = core_.get_cursor_pos();
        cursor_pos_.x() = std::max(0.f, cursor_pos_.x());
      }
    }

    void update_sound()
    {
      static int frame_index = 1;
      frame_index = frame_index == 0 ? 1 : 0;

      audio::engine::erase_finished_audio_on_queue(source_);

      if (audio::engine::get_audio_count_on_queue(source_) > queue_capacity_)
        return;

      float* raw_data = field_->get_sound_buffer(frame_index);
      float raw_i = 0.f;
      while (raw_i < update_per_frame_) {
        segment_.emplace_back(static_cast<ALshort>(raw_data[int(raw_i)] * amplify_));
        raw_i += 128000.f / 44100.f;
      }

      seg_frame_index++;

      if (seg_frame_index == 3) {
        seg_frame_index = 0;
        audio::audio_data data;
        data.set_sampling_rate(44100)
          .set_format(AL_FORMAT_MONO16)
          .set_data(segment_);
        audio::engine::bind_audio_to_buffer(data);
        audio::engine::queue_buffer_to_source(source_, data.get_buffer_id());
        segment_.clear();

        if (audio::engine::get_audio_count_on_queue(source_) == 2 && !started_) {
          audio::engine::play_audio_from_source(source_);
          started_ = true;
        }
      }
    }

    u_ptr<physics::fdtd2_field> field_;
    u_ptr<physics::fdtd2_field> staging_field_;
    bool wait_for_construction_ = false;

    float x_len_       = 0.6f;
    float y_len_       = 0.3f;
    float sound_speed_ = 340.f;
    float rho_         = 1.1f;
    int   update_per_frame_;
    float mouth_pressure_ = 4000.f;
    float amplify_ = 100.f;

    // GUI
    static bool button_pressed_;
    vec2 cursor_pos_;

    // AL
    audio::source_id source_;
    std::vector<ALshort> segment_;
    int seg_frame_index = 0; // mod 3
    bool started_ = false;
    int queue_capacity_ = 3;

    // arduino
    serial_com serial_;
};

bool fdtd_compute::button_pressed_ = false;

} // namespace hnll

int main() {
  hnll::fdtd_compute engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}