// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/compute_shader.hpp>
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>

namespace hnll {

constexpr float BORE_LENGTH = 0.5f;
constexpr float DX  = 3.83e-3;
constexpr float DT  = 1e-5;
constexpr float RHO = 1.1f;
constexpr float SOUND_SPEED = 340.f;
constexpr float V_FAC = DT / (RHO * DX);
constexpr float P_FAC = DT / RHO * SOUND_SPEED * SOUND_SPEED / DX;
constexpr uint32_t FDTD_FRAME_BUFFER_COUNT = 2;
constexpr uint32_t AUDIO_FRAME_BUFFER_COUNT = 2;
constexpr uint32_t PML_COUNT = 6;
constexpr uint32_t SAMPLING_RATE = 44100;
constexpr float    AUDIO_FPS = 1.f / DT;
constexpr float    GRAPHICS_FPS = 30;
int UPDATE_PER_FRAME = static_cast<int>(AUDIO_FPS / GRAPHICS_FPS);

std::vector<graphics::binding_info> desc_bindings = {
  { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

// temp
float shader_debug = 1.f;

float calc_y(float x)
{
  // straight
//  return 0.007f;
  // sine
//  return 0.02f + 0.01f * std::sin(50.f * x);
  // stairs
//  return 0.007f + 0.003f * (x > 0.15f) + 0.003f * (x > 0.30f);
  // cone
//  return 0.007f + 0.06f * x;
  // exponential
  return 0.007f + 0.001f * std::exp(40.f * std::max(x - 0.4f, 0.f));
}

// push constant, particle
#include "common.h"

class fdtd_1d_field
{
  public:
    fdtd_1d_field() : device_(utils::singleton<graphics::device>::get_single_ptr())
    {
      // calc constants
      main_grid_count_ = static_cast<int>(BORE_LENGTH / DX);
      whole_grid_count_ = PML_COUNT + main_grid_count_;

      // setup desc sets
      desc_pool_ = graphics::desc_pool::builder(*device_)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20)
        .build();

      graphics::desc_set_info set_info { desc_bindings };

      desc_sets_ = graphics::desc_sets::create(
        *device_,
        desc_pool_,
        { set_info, set_info, set_info }, // pv, sound, field
        FDTD_FRAME_BUFFER_COUNT
      );

      // set pv buffer
      {
        // initial pressure
        std::vector<particle> particles(whole_grid_count_ + 1, particle(0.f, 0.f, 0.f, 0.f));

        for (int i = 0; i < FDTD_FRAME_BUFFER_COUNT; i++) {
          auto buffer = graphics::buffer::create_with_staging(
            *device_,
            sizeof(particle) * particles.size(),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            particles.data()
          );
          desc_sets_->set_buffer(PV_DESC_SET_ID, 0, i, std::move(buffer));
        }
      }

      // set field buffer (y_offset and pml)
      // field doesn't have the prev buffer
      {
        // initial pressure
        std::vector<field_element> field_elements(whole_grid_count_ + 1, field_element(0.f, 0.f));
        // calc initial y offsets
        // TODO : set open space
        for (int i = 0; i < whole_grid_count_; i++) {
          field_elements[i].y_offset = calc_y(DX * i);
        }

        float bell_radius = field_elements[main_grid_count_ - 1].y_offset;
        float input_radius = field_elements[0].y_offset;
        amp_modify_ = std::pow(bell_radius, 2.f) / std::pow(input_radius, 2.f);

        // set pml
//        for (int i = 0; i < PML_COUNT; i++) {
//          field_elements[whole_grid_count_ - 1 - i].pml = float(PML_COUNT - i) / float(PML_COUNT);
//        }

        for (int i = 0; i < FDTD_FRAME_BUFFER_COUNT; i++) {
          auto buffer = graphics::buffer::create(
            *device_,
            sizeof(field_element) * field_elements.size(),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            field_elements.data()
          );

          desc_sets_->set_buffer(FIELD_DESC_SET_ID, 0, i, std::move(buffer));
        }
      }

      // set sound buffer
      {
        std::vector<float> sound(UPDATE_PER_FRAME, 0.f);

        for (int i = 0; i < FDTD_FRAME_BUFFER_COUNT; i++) {
          auto buffer = graphics::buffer::create(
            *device_,
            sizeof(float) * sound.size(),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sound.data()
          );

          // mapped memory
          if (i < AUDIO_FRAME_BUFFER_COUNT)
            sound_buffers_[i] = reinterpret_cast<float *>(buffer->get_mapped_memory());

          desc_sets_->set_buffer(SOUND_DESC_SET_ID, 0, i, std::move(buffer));
        }
      }

      desc_sets_->build();
    }

    void update_field()
    {
      duration_ += DT;
      frame_index_ = frame_index_ == FDTD_FRAME_BUFFER_COUNT - 1 ? 0 : frame_index_ + 1;
    }

    void update_audio()
    {
      audio_frame_index_ = audio_frame_index_ == AUDIO_FRAME_BUFFER_COUNT - 1 ? 0 : audio_frame_index_ + 1;
    }

    float get_duration()         const { return duration_; }
    int   get_main_grid_count()  const { return main_grid_count_; }
    int   get_whole_grid_count() const { return whole_grid_count_; }
    float get_amp_modify()       const { return amp_modify_; }
    float* get_mouth_pressure_p()      { return &mouth_pressure_; }

    std::vector<VkDescriptorSet> get_frame_desc_sets()
    {
      std::array<std::vector<VkDescriptorSet>, FDTD_FRAME_BUFFER_COUNT> desc_sets;
      for (int i = 0; i < FDTD_FRAME_BUFFER_COUNT; i++) {
        desc_sets[i] = desc_sets_->get_vk_desc_sets(i);
      }

      std::vector<VkDescriptorSet> ret;
      for (int i = 0; i < FDTD_FRAME_BUFFER_COUNT; i++) {
        ret.emplace_back(desc_sets[(FDTD_FRAME_BUFFER_COUNT - frame_index_ + i) % FDTD_FRAME_BUFFER_COUNT][PV_DESC_SET_ID]);
      }
      ret.emplace_back(desc_sets[0][FIELD_DESC_SET_ID]);
      ret.emplace_back(desc_sets[audio_frame_index_][SOUND_DESC_SET_ID]);

      return ret;
    }

    float* get_latest_sound_buffer()
    {
      return sound_buffers_[audio_frame_index_];
    }

  private:
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    utils::single_ptr<graphics::device> device_;

    float* sound_buffers_[AUDIO_FRAME_BUFFER_COUNT];

    int main_grid_count_;
    int whole_grid_count_;
    float duration_ = 0.f;
    float mouth_pressure_ = 0.f;

    int frame_index_ = 0;
    int audio_frame_index_ = 0;
    float amp_modify_ = 1.f;
};

DEFINE_SHADING_SYSTEM(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR_DECL(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
    : field_(utils::singleton<fdtd_1d_field>::build_instance()){}

    void setup()
    {
      auto device = utils::singleton<graphics::device>::get_single_ptr();
      desc_layout_ = graphics::desc_layout::create_from_bindings(*device, desc_bindings);
      auto vk_layout = desc_layout_->get_descriptor_set_layout();

      pipeline_layout_ = create_pipeline_layout<fdtd_push>(
        static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT),
        std::vector<VkDescriptorSetLayout>(FDTD_FRAME_BUFFER_COUNT + 2, vk_layout)
      );

      pipeline_ = create_pipeline(
        pipeline_layout_,
        game::graphics_engine_core::get_default_render_pass(),
        "/examples/fdtd_compute/fdtd_1d/shaders/spv/",
        { "fdtd_1d.vert.spv", "fdtd_1d.frag.spv" },
        { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
        graphics::pipeline::default_pipeline_config_info()
      );
    }

    void render(const utils::graphics_frame_info& frame_info)
    {
      set_current_command_buffer(frame_info.command_buffer);
      bind_pipeline();

      // push constants
      fdtd_push push;
      auto window_size = game::gui_engine::get_viewport_size();
      push.window_size = { window_size.x, window_size.y };
      push.len = BORE_LENGTH;
      push.dx  = DX;
      push.main_grid_count = field_->get_main_grid_count();
      push.whole_grid_count = field_->get_whole_grid_count();
      push.v_fac = V_FAC;
      push.p_fac = P_FAC;
      bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

      // desc sets
      bind_desc_sets(field_->get_frame_desc_sets());

      vkCmdDraw(current_command_, 6, 1, 0, 0);
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

DEFINE_COMPUTE_SHADER(fdtd_1d_compute)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(fdtd_1d_compute) : field_(utils::singleton<fdtd_1d_field>::get_single_ptr()) {}

    void setup()
    {
      desc_layout_ = graphics::desc_layout::create_from_bindings(*device_, desc_bindings);
      auto vk_layout = desc_layout_->get_descriptor_set_layout();

      pipeline_ = create_pipeline<fdtd_push>(
        utils::get_engine_root_path() + "/examples/fdtd_compute/fdtd_1d/shaders/spv/fdtd_1d.comp.spv",
        std::vector<VkDescriptorSetLayout>(FDTD_FRAME_BUFFER_COUNT + 2, vk_layout)
      );
    }

    void render(const utils::compute_frame_info& info)
    {
      auto command = info.command_buffer;
      bind_pipeline(command);

      // push constants
      fdtd_push push;
      auto window_size = game::gui_engine::get_viewport_size();
      push.window_size = { window_size.x, window_size.y };
      push.len = BORE_LENGTH;
      push.dx  = DX;
      push.v_fac = V_FAC;
      push.p_fac = P_FAC;
      push.main_grid_count = 80;field_->get_main_grid_count();
      push.whole_grid_count = field_->get_whole_grid_count();
      push.mouth_pressure = *(field_->get_mouth_pressure_p());
      push.debug = shader_debug;

      // barrier for pressure, velocity update synchronization
      VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
      };

      static float phase = 0.f;

      for (int i = 0; i < UPDATE_PER_FRAME; i++) {
        // push constants
        push.phase = phase;
        push.sound_buffer_id = i;
        bind_push(
          command,
          static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_COMPUTE_BIT),
          push
        );

        // desc sets
        bind_desc_sets(command, field_->get_frame_desc_sets());
        dispatch_command(
          command,
          (int(field_->get_whole_grid_count()) + fdtd1_local_size_x - 1) / fdtd1_local_size_x,
          1,
          1
        );

        if (i != UPDATE_PER_FRAME - 1) {
          vkCmdPipelineBarrier(
            command,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_DEVICE_GROUP_BIT,
            1, &barrier, 0, nullptr, 0, nullptr
          );
        }

        field_->update_field();
        phase += 2.f * M_PI * 440.f * DT;
        phase = std::fmod(phase, 2.f * M_PI);
      }

      field_->update_audio();
    }

  private:
    utils::single_ptr<fdtd_1d_field> field_;
};

SELECT_ACTOR(game::no_actor);
SELECT_SHADING_SYSTEM(fdtd_1d_shader);
SELECT_COMPUTE_SHADER(fdtd_1d_compute);

DEFINE_ENGINE(curved_fdtd_1d)
{
  public:
    explicit ENGINE_CTOR(curved_fdtd_1d), field_(utils::singleton<fdtd_1d_field>::build_instance())
    {
      set_max_fps(30.f);

      // audio setup
      audio::engine::start_hae_context();
      source_ = audio::engine::get_available_source_id();
    }

    void cleanup() { audio::engine::kill_hae_context(); }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %f", 1.f / dt);
      ImGui::Text("duration : %f", field_->get_duration());
      ImGui::SliderFloat("amp", &amplify_, 1, 100);
      ImGui::SliderFloat("mouth_pressure", field_->get_mouth_pressure_p(), 0, 5000);
      ImGui::SliderInt("update per frame", &UPDATE_PER_FRAME, 2, 3030);
      ImGui::SliderFloat("shader debug", &shader_debug, 1.f, 30.f);
      ImGui::End();

      update_sound();
    }

  private:
    void update_sound()
    {
      audio::engine::erase_finished_audio_on_queue(source_);

      static const int queue_capacity = 3;
      if (audio::engine::get_audio_count_on_queue(source_) > queue_capacity)
        return;

      float* raw_data = field_->get_latest_sound_buffer();

      //temp
      std::vector<float> watch(UPDATE_PER_FRAME);
      for (int i = 0; i < UPDATE_PER_FRAME; i++) {
        watch[i] = raw_data[i];
      }

      float raw_i = 0.f;
      float amp = amplify_ * field_->get_amp_modify();
      while (raw_i < float(UPDATE_PER_FRAME)) {
        segment_.emplace_back(static_cast<ALshort>(raw_data[int(raw_i)] * amp));
        raw_i += AUDIO_FPS / SAMPLING_RATE;
      }

      seg_frame_index_++;

      // register the data to the audio engine
      if (seg_frame_index_ == 3) {
        seg_frame_index_ = 0;
        audio::audio_data data;
        data.set_sampling_rate(SAMPLING_RATE)
          .set_format(AL_FORMAT_MONO16)
          .set_data(segment_);
        audio::engine::bind_audio_to_buffer(data);
        audio::engine::queue_buffer_to_source(source_, data.get_buffer_id());
        segment_.clear();

        if (!started_ && audio::engine::get_audio_count_on_queue(source_) == 3) {
          audio::engine::play_audio_from_source(source_);
          started_ = true;
        }
      }
    }

    utils::single_ptr<fdtd_1d_field> field_;

    // audio
    audio::source_id source_;
    std::vector<ALshort> segment_;
    int seg_frame_index_ = 0; // mod 3
    bool started_ = false;
    float amplify_ = 3.f;
};

} // namespace hnll

int main()
{
  hnll::utils::vulkan_config config;
  config.present = hnll::utils::present_mode::IMMEDIATE;
  config.enable_validation_layers = false;
  hnll::curved_fdtd_1d app("1d fdtd", config);

  try { app.run(); }
  catch(const std::exception& e) { std::cerr << e.what() << std::endl; }
}