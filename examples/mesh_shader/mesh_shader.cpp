// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/component.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/actors/default_camera.hpp>

// std
#include <iostream>

namespace hnll {

DEFINE_COMPONENT(dummy_component)
{
  public:
    void update(const float& dt) {}
};

DEFINE_ACTOR(dummy_actor, dummy_component)
{

};

SELECT_SHADING_SYSTEM(shading_systems, game::grid_shading_system);
SELECT_ACTOR(actors, game::default_camera, dummy_actor);

DEFINE_ENGINE(my_engine, shading_systems, actors)
{
  public:
    my_engine()
    {
      add_actor<dummy_actor>();
      add_actor<game::default_camera>();
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