// hnll
#include <game/engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// lib
#include <imgui/imgui.h>

namespace hnll {

SELECT_SHADING_SYSTEM(graphics_shaders, physics::fdtd2_shading_system);
SELECT_COMPUTE_SHADER(compute_shaders, physics::fdtd2_compute_shader);

DEFINE_ENGINE_WITH_COMPUTE(fdtd_compute, graphics_shaders, game::no_actor, compute_shaders)
{
  public:
    fdtd_compute()
    {
      physics::fdtd_info info = {
        10.f,
        10.f,
        314.f,
        1.f,
        1.f,
        4000.f
      };

      field_ = physics::fdtd2_field::create(info);
    }

    void update_this(float dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %d", int(1.f/dt));
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