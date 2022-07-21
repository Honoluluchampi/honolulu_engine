#pragma once

// hnll
#include <game/components/renderable_component.hpp>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {
namespace game {

// pointLight can be treated as gameObject
struct point_light_info { float light_intensity = 1.0f; };

class point_light_component : public renderable_component
{
  public:
    point_light_component() : renderable_component(render_type::POINT_LIGHT) {}
    ~point_light_component(){}
    static s_ptr<point_light_component> create_point_light
      (float intensity = 10.f, float radius = 0.1f, Eigen::Vector3f color = Eigen::Vector3f::Identity());

    // getter
    point_light_info get_light_info() { return light_info_; }
    Eigen::Vector3f get_color() { return color_; }

    // setter
    void set_color(const Eigen::Vector3f& color) { color_ = color; }
    void set_light_info(const point_light_info& info) { light_info_ = info; }
    void set_radius(float radius) { this->set_scale(Eigen::Vector3f(radius, radius, radius)); }
  
  private:
    point_light_component(const point_light_component&) = delete;
    point_light_component& operator=(const point_light_component&) = delete;
    point_light_info light_info_{};
    Eigen::Vector3f color_ = Eigen::Vector3f::Zero();
};

} // namespace game
} // namespace hnll