// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/component.hpp>
#include <game/shading_systems/grid_shading_system.hpp>

// std
#include <iostream>

#define DEFINE_ENGINE(new_engine, shading_systems, actors) class new_engine : public game::engine_base<new_engine, shading_systems, actors>
#define DEFINE_ACTOR(new_actor, ...) class new_actor : public game::actor_base<new_actor, __VA_ARGS__>
#define DEFINE_COMPONENT(new_comp) class new_comp : public game::component_base<new_comp>
#define SELECT_SHADING_SYSTEM(name, ...) using name = game::shading_system_list<__VA_ARGS__>
#define SELECT_ACTOR(name, ...) using name = game::actor_list<__VA_ARGS__>

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
SELECT_ACTOR(actors, dummy_actor);

DEFINE_ENGINE(my_engine, shading_systems, actors)
{
  public:
//    void update_this(const float& dt);
};

} // namespace hnll

int main() {
  hnll::my_engine engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}