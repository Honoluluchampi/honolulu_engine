// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/component.hpp>
#include <game/shading_systems/grid_shading_system.hpp>

// std
#include <iostream>

#define DEFINE_ENGINE( new_engine, shading_systems, actors ) new_engine : public game::engine_base<new_engine, shading_systems, actors>
#define DEFINE_ACTOR( new_actor, ... ) new_actor : public game::actor_base<new_actor, __VA_ARGS__>
#define DEFINE_COMPONENT( new_comp ) new_comp : public game::component_base<new_comp>

namespace hnll {

class DEFINE_COMPONENT(dummy_component)
{
  public:
    void update(const float& dt) {}
};

class DEFINE_ACTOR(dummy_actor, dummy_component)
{

};

using shading_systems = game::shading_system_list<game::grid_shading_system>;
using actors = game::actor_list<dummy_actor>;

class DEFINE_ENGINE(my_engine, shading_systems, actors)
{
  private:
    void update_this(const float& dt) {}
};

} // namespace hnll

int main() {
  hnll::my_engine engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}