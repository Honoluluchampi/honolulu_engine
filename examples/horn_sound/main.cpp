// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(horn)
{
  horn() : game::pure_actor_base<horn>() {

  }

  float length;
  float width;
};

DEFINE_SHADING_SYSTEM(horn_shading_system, )
{};

} // namespace hnll

int main()
{

}