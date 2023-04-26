// hnll
#include <game/engine.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

namespace hnll {

SELECT_SHADING_SYSTEM(graphics_shaders, game::grid_shading_system); //physics::fdtd2_shading_system);
// TODO : replace with dummy_actor
SELECT_COMPUTE_SHADER(compute_shaders, physics::fdtd2_compute_shader);

DEFINE_ENGINE_WITH_COMPUTE(fdtd_compute, graphics_shaders, game::no_actor, compute_shaders)
{
  public:
    fdtd_compute()
    {
      physics::fdtd_info info = {
        1.f,
        1.f,
        314.f,
        1.f,
        1.f,
        4000.f
      };

      field_ = physics::fdtd2_field::create(info);
    }

  private:
    s_ptr<physics::fdtd2_field> field_;
};

} // namespace hnll

int main() {
  hnll::fdtd_compute engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}