// hnll
#include <geometry/intersection.hpp>
#include <geometry/bounding_volume.hpp>
#include <geometry/primitives.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll::geometry {

using namespace hnll::geometry::intersection;

vec3d point5 = {1.f, 0.f, 0.f};
vec3d point6 = {6.f, 0.f, 0.f};
vec3d point3 = {3.f, 0.f, 0.f};
vec3d point4 = {6.f, 3.f, 8.f};
auto sp3 = bounding_volume(point5, 3.f);
auto sp4 = bounding_volume(point6, 4.f);
auto aabb1 = bounding_volume(point3, {3.f, 2.f, 4.f});
auto aabb2 = bounding_volume(point4, {1.f, 0.5f, 4.f});

TEST(intersection, sphere_sphere) {
  // intersection
  EXPECT_TRUE(test_bv_intersection(sp3, sp4));
  sp4.set_center_point({1.f, 7.f, 0.f});
  EXPECT_TRUE(test_bv_intersection(sp3, sp4));
  sp4.set_center_point({1.f, 8.f, 0.f});
  EXPECT_FALSE(test_bv_intersection(sp3, sp4));
}

TEST(intersection, aabb_aabb) {
  EXPECT_FALSE(test_bv_intersection(aabb1, aabb2));
  aabb2.set_aabb_radius({1.f, 1.f, 4.f});
  EXPECT_TRUE(test_bv_intersection(aabb1, aabb2));
  aabb2.set_aabb_radius({1.f, 1.1f, 4.f});
  EXPECT_TRUE(test_bv_intersection(aabb1, aabb2));
}

perspective_frustum frustum = {M_PI / 2.f, M_PI / 2.f, 3, 10};

TEST(intersection, sphere_frustum) {
  // far test
  auto sphere = bounding_volume{vec3d(0.f, 0.f, 10.f), 3};
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({0.f, 0.f, 13.f});
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({0.f, 0.f, 13.1f});
  EXPECT_FALSE(test_sphere_frustum(sphere, frustum));

  // near test
  sphere.set_center_point({0.f, 0.f, 0.f});
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({0.f, 0.f, -0.1f});
  EXPECT_FALSE(test_sphere_frustum(sphere, frustum));

  // right test
  double r32 = 3.f / std::sqrt(2.f);
  double eps = 0.0001f;
  sphere.set_center_point({5.f + r32 - eps, 0.f, 5.f - r32 + eps});
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({5.f + r32 - 0.1f, 0.f, 5.f - r32 + eps});
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({5.f + r32 + 0.1f, 0.f, 5.f - r32 + eps});
  EXPECT_FALSE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({5.f + r32 - eps, 0.f, 5.f - r32 + 0.1f});
  EXPECT_TRUE(test_sphere_frustum(sphere, frustum));
  sphere.set_center_point({5.f + r32 - eps, 0.f, 5.f - r32 - 0.1f});
  EXPECT_FALSE(test_sphere_frustum(sphere, frustum));
}

} // namespace hnll::geometry;