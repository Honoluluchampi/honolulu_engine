// hnll
#include <game/components/point_light_component.hpp>

namespace hnll {
namespace game {

template <typename T> using s_ptr = std::shared_ptr<T>;

s_ptr<point_light_component> point_light_component::create_point_light(float intensity, float radius, Eigen::Vector3d color)
{
  auto light = std::make_shared<point_light_component>();
  light->color_ = color;
  light->set_scale(Eigen::Vector3d(radius, radius, radius));
  light->light_info_.light_intensity = intensity;
  return std::move(light);
}

}
} // namespace hnll