// hnll
#include <game/engine.hpp>
#include "include/fdtd_cylindrical_field.hpp"
#include "include/fdtd_cylindrical_compute_shader.hpp"
#include "include/fdtd_cylindrical_shading_system.hpp"
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>

// std
#include <thread>

// lib
#include "imgui/imgui.h"

constexpr int AUDIO_FRAME_RATE = 128000;
constexpr int DEFAULT_UPDATE_PER_FRAME = 4268;

namespace hnll {

SELECT_SHADING_SYSTEM(fdtd_cylindrical_shading_system);
SELECT_COMPUTE_SHADER(fdtd_cylindrical_compute_shader);
SELECT_ACTOR(fdtd_cylindrical_field);

DEFINE_ENGINE(fdtd_cylindrical)
{
  public:
    ENGINE_CTOR(fdtd_cylindrical)
    {
      set_max_fps(30.f);
      auto game_fps = get_max_fps();

      update_per_frame_ = std::ceil(float(AUDIO_FRAME_RATE) / game_fps);
      if (update_per_frame_ % 2 != 0)
        update_per_frame_ += 1;

      fdtd_info info = {
        .z_len = z_len_,
        .r_len = r_len_,
        .sound_speed = sound_speed_,
        .rho = rho_,
        .pml_count = 10,
        .update_per_frame = update_per_frame_,
        .fcm_on = false
      };

      field_ = fdtd_cylindrical_field::create(info);
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
      ImGui::Text("z grid : %d", field_->get_z_grid_count());
      ImGui::Text("r grid : %d", field_->get_r_grid_count());
      ImGui::Text("dt : %f", field_->get_dt());
      ImGui::Text("grid size : %f", field_->get_dz());
      ImGui::Text("duration : %f", field_->get_duration());
      ImGui::Text("cursor x : %f", cursor_pos_.x());
      ImGui::Text("cursor y : %f", cursor_pos_.y());

      ImGui::SliderFloat("z length", &z_len_, 0.1f, 0.8f);
      ImGui::SliderFloat("max radius", &r_len_, 0.1f, 0.4f);
      ImGui::SliderInt("update per frame : %d", &update_per_frame_, 3, DEFAULT_UPDATE_PER_FRAME);
      ImGui::SliderFloat("input pressure : %f", &mouth_pressure_, 0.f, 3800.f);
      ImGui::SliderFloat("amplify : %f", &amplify_, 0.f, 300.f);
      ImGui::SliderFloat("fcm freq : %f", &fcm_freq_, 20.f, 1000.f);

      field_->add_duration();
      field_->set_update_per_frame(update_per_frame_);
      field_->set_mouth_pressure(mouth_pressure_);
      field_->set_fcm_freq(fcm_freq_);

      update_gui();
      update_sound();

      ImGui::End();
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
        cursor_pos_ = core_->get_cursor_pos();
        cursor_pos_.x() = std::max(0.f, cursor_pos_.x());
      }
    }

    void update_sound()
    {
      audio::engine::erase_finished_audio_on_queue(source_);

      if (audio::engine::get_audio_count_on_queue(source_) > queue_capacity_)
        return;

      float* raw_data = field_->get_sound_buffer();
      float raw_i = 0.f;
      while (raw_i < update_per_frame_) {
        segment_.emplace_back(static_cast<ALshort>(
          std::clamp(raw_data[int(raw_i)] * amplify_,
           float(std::numeric_limits<ALshort>::min()),
           float(std::numeric_limits<ALshort>::max())
          )));
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

    u_ptr<fdtd_cylindrical_field> field_;
    u_ptr<fdtd_cylindrical_field> staging_field_;

    float z_len_       = 0.6f;
    float r_len_       = 0.15f; // radius
    float sound_speed_ = 340.f;
    float rho_         = 1.1f;
    int   update_per_frame_;
    float mouth_pressure_ = 3800.f;
    float amplify_ = 100.f;
    float fcm_freq_ = 400.f;

    // GUI
    static bool button_pressed_;
    vec2 cursor_pos_;

    // AL
    audio::source_id source_;
    std::vector<ALshort> segment_;
    int seg_frame_index = 0; // mod 3
    bool started_ = false;
    int queue_capacity_ = 3;
};

bool fdtd_cylindrical::button_pressed_ = false;

} // namespace hnll

int main() {

  hnll::utils::vulkan_config config;
  config.enable_validation_layers = false;

  hnll::fdtd_cylindrical engine("fdtd_cylindrical", config);

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}