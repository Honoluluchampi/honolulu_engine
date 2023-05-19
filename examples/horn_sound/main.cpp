// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>
#include <game/modules/gui_engine.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(horn)
{
  public:
    horn() : game::pure_actor_base<horn>() {}

    void bind(VkCommandBuffer command) {}
    void draw(VkCommandBuffer command) { vkCmdDraw(command, 6, 1, 0, 0); }

    // to use as renderable component
    uint32_t            get_rc_id() const { return id_; }
    utils::shading_type get_shading_type() const { return utils::shading_type::UNIQUE; }

    float length = 100.f;
    float width = 20.f;
    uint32_t id;
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
      target_ = std::make_shared<horn>();
      horn_shading_system::set_target(target_);
    }

    // GUI
    void update_this(float dt)
    {
      ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      ImGui::Text("horn length : %d mm", int(target_->length));
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