// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>
#include <game/modules/gui_engine.hpp>
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>
#include <audio/utils.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(horn)
{
  public:
    horn() : game::pure_actor_base<horn>()
      { source = audio::engine::get_available_source_id(); }

    void bind(VkCommandBuffer command) {}
    void draw(VkCommandBuffer command) { vkCmdDraw(command, 6, 1, 0, 0); }

    void update_this(const float& dt)
    {
      auto audio = audio::utils::create_sine_wave(
        dt + 1,
        length
      );

      audio::audio_data data;
      data
        .set_sampling_rate(44100)
        .set_format(AL_FORMAT_MONO16)
        .set_data(std::move(audio));

      audio::engine::erase_finished_audio_on_queue(source);

      if (audio::engine::get_audio_count_on_queue(source) <= queue_capability) {
        audio::engine::bind_audio_to_buffer(data);
        audio::engine::queue_buffer_to_source(source, data.get_buffer_id());
      }

      if (!is_ringing) {
        audio::engine::play_audio_from_source(source);
        is_ringing = true;
      }
    }

    // to use as renderable component
    uint32_t            get_rc_id() const { return id_; }
    utils::shading_type get_shading_type() const { return utils::shading_type::UNIQUE; }

    float length = 100.f;
    float width = 20.f;
    uint32_t id;

    // audio queue is constructed on this queue
    audio::source_id source;
    bool is_ringing = false;
    int queue_capability = 3;
};

// include push constant (shared with the shader)
#include "common.h"

DEFINE_SHADING_SYSTEM(horn_shading_system, horn)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(horn_shading_system, horn);

    void setup()
    {
      pipeline_layout_ = create_pipeline_layout<horn_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        {}
      );
      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/horn_sound/shaders/spv/",
        { "horn.vert.spv", "horn.frag.spv" },
        { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
        graphics::pipeline::default_pipeline_config_info()
      );
    }

    void render(const utils::graphics_frame_info& info)
    {
      if (target_ != nullptr) {
        auto command = info.command_buffer;
        set_current_command_buffer(command);
        bind_pipeline();

        // prepare push constants
        auto window_size = game::gui_engine::get_viewport_size();
        horn_push push;
        push.h_length = target_->length;
        push.h_width  = target_->width;
        push.w_width  = window_size.x;
        push.w_height = window_size.y;
        bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        target_->draw(command);
      }
    }

    static void set_target(const s_ptr<horn>& target) { target_ = target; }

  private:
    static s_ptr<horn> target_;
};

s_ptr<horn> horn_shading_system::target_;

SELECT_ACTOR(horn);
SELECT_SHADING_SYSTEM(horn_shading_system);
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(horn_sound)
{
  public:
    explicit ENGINE_CTOR(horn_sound)
    {
      // start audio engine
      audio::engine::start_hae_context();

      target_ = std::make_shared<horn>();
      horn_shading_system::set_target(target_);
    }

    ~horn_sound()
    { audio::engine::kill_hae_context(); }

    // GUI
    void update_this(float dt)
    {
      target_->update_this(dt);
      ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      ImGui::Text("horn length : %d mm", int(target_->length));
      ImGui::SliderFloat("length", &target_->length, 1.f, 500.f);
      ImGui::SliderFloat("width",  &target_->width, 1.f, 500.f);
      ImGui::End();
    }

  private:
    s_ptr<horn> target_;
};

} // namespace hnll

int main()
{
  hnll::horn_sound engine("horn sound");
  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}