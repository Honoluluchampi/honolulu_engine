// hnll
#include <game/actors/point_light_manager.hpp>
#include <graphics/frame_info.hpp>

namespace hnll {
namespace game {

point_light_manager::point_light_manager(hnll::graphics::global_ubo& ubo) : 
  actor(), ubo_(ubo)
{
}

void point_light_manager::update_actor(float dt)
{
  int light_index = 0;
  auto light_rotation = Eigen::AngleAxisd(-dt, Eigen::Vector3d::UnitY());
  for (auto& kv : light_comp_map_) {
    auto light_comp = kv.second;
    // update light position
    // light_comp->get_transform().translation = glm::vec3(light_rotation * glm::vec4(light_comp->get_transform().translation, 1.f));
    // copy light data to ubo 
    ubo_.point_lights[light_index].position << light_comp->get_transform_sp()->translation, 1.f;
    ubo_.point_lights[light_index].color << light_comp->get_color(), light_comp->get_light_info().light_intensity;
    light_index++;
  }
  ubo_.lights_count = light_index;
}

} // namespace game
} // namespace hnll