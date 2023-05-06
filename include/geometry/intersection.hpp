#pragma once

// hnll
#include <geometry/bounding_volume.hpp>
#include <utils/common_alias.hpp>

// std
#include <vector>
#include <memory>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll{

namespace geometry {

// forward declaration
class perspective_frustum;
struct ray;
struct vertex;
struct plane;

namespace intersection {

// testing functions for culling algorithms
double test_sphere_frustum(const b_sphere& sphere, const perspective_frustum& frustum);

// returns the length of intersection
double test_bv_intersection(const aabb& aabb_a, const aabb& aabb_b);
double test_bv_intersection(const aabb& aabb, const b_sphere& sphere);
double test_bv_intersection(const b_sphere& sphere_a, const b_sphere& sphere_b);
}; // namespace intersection

// helper functions
double distance_point_to_plane(const vec3d& q, const plane& p);

}} // namespace hnll::geometry