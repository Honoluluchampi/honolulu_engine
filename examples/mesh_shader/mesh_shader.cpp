// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/component.hpp>
#include <game/shading_systems/alias.hpp>
#include <game/actors/default_camera.hpp>
#include <game/renderable_component.hpp>

// std
#include <iostream>

namespace hnll {

DEFINE_PURE_ACTOR(mesh_actor)
{
  public:
    mesh_actor() { mesh_model_ = game::static_mesh_comp::create(*this, "bunny.obj"); }
  private:
    u_ptr<game::static_mesh_comp> mesh_model_;
};

SELECT_SHADING_SYSTEM(shading_systems, game::grid_shading_system);
SELECT_ACTOR(actors, game::default_camera);

DEFINE_ENGINE(my_engine, shading_systems, actors)
{
  public:
    my_engine()
    {

      add_update_target_directly<game::default_camera>();
    }
};

} // namespace hnll

int main() {
  hnll::my_engine engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}