// hnll
#include <game/engine.hpp>
#include <game/actors/default_camera.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>
#include <imgui/imgui.h>

namespace hnll {

SELECT_SHADING_SYSTEM(graphics_shaders,
  game::grid_shading_system,
  physics::cloth_compute_shading_system);

SELECT_ACTOR(actors, game::default_camera);

SELECT_COMPUTE_SHADER(compute_shaders, physics::cloth_compute_shader);

DEFINE_ENGINE_WITH_COMPUTE(cloth_simulation, graphics_shaders, actors, compute_shaders)
{
  public:
    cloth_simulation()
    {
      add_update_target_directly<game::default_camera>();
      cloth_ = physics::mass_spring_cloth::create(256, 256, 10, 10);
    }
    ~cloth_simulation() {}

    void update_this(const float& dt)
    {
      dt_ = dt;
      ImGui::Begin("fps");
      ImGui::Text("fps : %d", static_cast<int>(1.f / dt_));
      ImGui::End();
    }

    void cleanup() { cloth_.reset(); }
  private:
    s_ptr<physics::mass_spring_cloth> cloth_;
    float dt_;
};

}

int main()
{
  hnll::cloth_simulation engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}