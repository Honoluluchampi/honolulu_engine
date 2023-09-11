// hnll
#include <game/engine.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <game/actor.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(mesh_particle)
{

};

// grid and mesh shader
SELECT_SHADING_SYSTEM(game::grid_shading_system, game::static_mesh_shading_system);
// only particle
SELECT_ACTOR(mesh_particle);
// no compute task
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(dancing_particles)
{
  public:
    explicit ENGINE_CTOR(dancing_particles){}
};

} // namespace hnll

int main()
{
  auto app = hnll::dancing_particles("dancing particles", hnll::utils::rendering_type::VERTEX_SHADING);

  try { app.run(); }
  catch (const std::exception& e) { throw e; }
}