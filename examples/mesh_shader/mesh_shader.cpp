// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/component.hpp>
#include <game/shading_systems/grid_shading_system.hpp>

// std
#include <iostream>

namespace hnll {

class dummy_component : public game::component_base<dummy_component>
{
  public:
    void update(const float& dt) {}
};

using dummy_actor = game::actor<dummy_component>;

using shading_system_list = game::shading_system_list<game::grid_shading_system>;
using actor_list = game::actor_list<dummy_actor>;

using mesh_shader = game::engine<shading_system_list, actor_list>;

} // namespace hnll

int main() {
  hnll::mesh_shader engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}