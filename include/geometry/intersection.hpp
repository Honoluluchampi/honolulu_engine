#pragma once

// hnll
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
class bounding_volume;
struct ray;
struct vertex;
struct plane;

namespace intersection {

// testing functions for culling algorithms
double test_sphere_frustum(const bounding_volume& sphere, const perspective_frustum& frustum);

// returns the length of intersection
double test_bv_intersection(const bounding_volume& a, const bounding_volume& b);
}; // namespace intersection

// helper functions
double distance_point_to_plane(const vec3d& q, const plane& p);

}} // namespace hnll::geometry