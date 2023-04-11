// hnll
#include <game/engine.hpp>
#include <game/actors/default_camera.hpp>
#include <game/compute_shader.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/swap_chain.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>

namespace hnll {

SELECT_SHADING_SYSTEM(graphics_shaders, game::grid_shading_system, game::static_mesh_shading_system, physics::cloth_compute_shading_system);
SELECT_ACTOR(actors, game::default_camera);
SELECT_COMPUTE_SHADER(compute_shaders, physics::cloth_compute_shader);

DEFINE_ENGINE_WITH_COMPUTE(cloth_compute, graphics_shaders, actors, compute_shaders)
{
  public:
    cloth_compute() { add_update_target_directly<game::default_camera>(); }
    ~cloth_compute() {}
  private:
};

}

int main()
{
  hnll::cloth_compute engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}