// hnll
#include <geometry/intersection.hpp>
#include <geometry/primitives.hpp>
#include <geometry/bounding_volume.hpp>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll::geometry {

double test_aabb_aabb(const bounding_volume &aabb_a, const bounding_volume &aabb_b)
{
  assert(aabb_a.get_bv_type() == bv_type::AABB);
  assert(aabb_b.get_bv_type() == bv_type::AABB);
  if (std::abs(aabb_a.get_world_center_point().x() - aabb_b.get_world_center_point().x()) > aabb_a.get_aabb_radius().x() + aabb_b.get_aabb_radius().x()) return false;
  if (std::abs(aabb_a.get_world_center_point().y() - aabb_b.get_world_center_point().y()) > aabb_a.get_aabb_radius().y() + aabb_b.get_aabb_radius().y()) return false;
  if (std::abs(aabb_a.get_world_center_point().z() - aabb_b.get_world_center_point().z()) > aabb_a.get_aabb_radius().z() + aabb_b.get_aabb_radius().z()) return false;
  return true;
}

double test_sphere_sphere(const bounding_volume &sphere_a, const bounding_volume &sphere_b)
{
  assert(sphere_a.get_bv_type() == bv_type::SPHERE);
  assert(sphere_b.get_bv_type() == bv_type::SPHERE);
  Eigen::Vector3d difference = sphere_a.get_world_center_point() - sphere_b.get_world_center_point();
  double distance2 = difference.dot(difference);
  float radius_sum = sphere_a.get_sphere_radius() + sphere_b.get_sphere_radius();
  return distance2 <= radius_sum * radius_sum;
}

// support functions for test_aabb_sphere
// prefix 'cp' is abbreviation of 'closest point'
vec3d cp_point_to_plane(const vec3d& q, const plane& p)
{
  // plane's normal must be normalized before this test
  float t = p.normal.dot(q - p.point);
  return q - t * p.normal;
}

double distance_point_to_plane(const vec3d& q, const plane& p)
{
  return p.normal.dot(q - p.point);
}

// caller of this function is responsible for insuring that the bounding_volume is aabb
vec3 cp_point_to_aabb(const vec3d& p, const bounding_volume& aabb)
{
  vec3 q;
  // TODO : simdlize
  for (int i = 0; i < 3; i++){
    float v = p[i];
    if (v < aabb.get_world_center_point()[i] - aabb.get_aabb_radius()[i]) v = aabb.get_world_center_point()[i] - aabb.get_aabb_radius()[i];
    else if (v > aabb.get_world_center_point()[i] + aabb.get_aabb_radius()[i]) v = aabb.get_world_center_point()[i] + aabb.get_aabb_radius()[i];
    q[i] = v;
  }
  return q;
}

// sq_dist is abbreviation of 'squared distance'
double sq_dist_point_to_aabb(const vec3d& p, const bounding_volume& aabb)
{
  double result = 0.0f;
  for (int i = 0; i < 3; i++) {
    float v = p[i];
    if (v < aabb.get_world_center_point()[i] - aabb.get_aabb_radius()[i]) result += std::pow(
        aabb.get_world_center_point()[i] - aabb.get_aabb_radius()[i] - v, 2);
    else if (v > aabb.get_world_center_point()[i] + aabb.get_aabb_radius()[i]) result += std::pow(v -
                                                                                                  aabb.get_world_center_point()[i] - aabb.get_aabb_radius()[i], 2);
  }
  return result;
}

double test_aabb_sphere(const bounding_volume &aabb, const bounding_volume &sphere)
{
  assert(aabb.get_bv_type() == bv_type::AABB);
  assert(sphere.get_bv_type() == bv_type::SPHERE);
  auto sq_dist = sq_dist_point_to_aabb(sphere.get_world_center_point(), aabb);
  return std::max(sphere.get_sphere_radius() - std::sqrt(sq_dist), static_cast<double>(0));
}

double intersection::test_sphere_frustum(const bounding_volume &sphere, const perspective_frustum &frustum)
{
  assert(sphere.get_bv_type() == bv_type::SPHERE);

  const auto  center = sphere.get_world_center_point().cast<double>();

  auto radii = sphere.get_aabb_radius();
  auto radius = std::sqrt(std::pow(radii.x(), 2) + std::pow(radii.y(), 2) + std::pow(radii.z(), 2));

  // TODO : simdlize
  // compare each distance with sphere radius;
  if (distance_point_to_plane(center, frustum.get_near())   < -radius) return false;
  if (distance_point_to_plane(center, frustum.get_far())    < -radius) return false;
  if (distance_point_to_plane(center, frustum.get_left())   < -radius) return false;
  if (distance_point_to_plane(center, frustum.get_right())  < -radius) return false;
  if (distance_point_to_plane(center, frustum.get_top())    < -radius) return false;
  if (distance_point_to_plane(center, frustum.get_bottom()) < -radius) return false;

  return true;
}

} // namespace hnll::physics