// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(horn)
{
  public:
    horn() : game::pure_actor_base<horn>()
    {

    }

    void bind(VkCommandBuffer command) {}
    void draw(VkCommandBuffer command) { vkCmdDraw(command, 6, 1, 0, 0); }

    // to use as renderable component
    uint32_t            get_rc_id() const { return id_; }
    utils::shading_type get_shading_type() const { return utils::shading_type::UNIQUE; }

  private:
    float length;
    float width;
    uint32_t id_;
};

DEFINE_SHADING_SYSTEM(horn_shading_system, horn)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(horn_shading_system, horn);

    void setup() {}
    void render(const utils::graphics_frame_info& info) {}
};

SELECT_ACTOR(horn);
SELECT_SHADING_SYSTEM(horn_shading_system);
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(horn_sound)
{

};

} // namespace hnll

int main()
{
  hnll::horn_sound engine;
  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}