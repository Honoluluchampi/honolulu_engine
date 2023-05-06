// hnll
#include <geometry/primitives.hpp>
#include <geometry/he_mesh.hpp>
#include <utils/utils.hpp>

// lib
#include <gtest/gtest.h>

const double EPSILON = 0.0001f;

namespace hnll::geometry {

// half edge -------------------------------------------------------------------------
bool eps_eq(const vec3d& a, const vec3d& b)
{
  return (abs(a.x()-b.x()) < EPSILON && abs(a.y()-b.y()) < EPSILON && abs(a.z()-b.z()) < EPSILON);
}

TEST(vert, ctor) {
  vertex v { vec3d{0.f, 1.f, 0.f}, 2 };
  EXPECT_EQ(v.position, vec3d(0.f, 1.f, 0.f));
  half_edge he { v, 4 };
  EXPECT_EQ(v.he_id, he.this_id);
}

TEST(common, id) {
  auto model = he_mesh::create();
  /*
   *  v0 - v2
   *   | /  |
   *  v1 - v3
   */
  vertex v0 { vec3d{ 0.f, 0.f, 0.f }, 0 };
  vertex v1 { vec3d{ 0.f, 1.f, 0.f }, 1 };
  vertex v2 { vec3d{ 1.f, 0.f, 0.f }, 2 };
  vertex v3 { vec3d{ 1.f, 1.f, 0.f }, 3 };
  auto fc_id0 = model->add_face(v0, v1, v2, 0);
  auto v = model->get_vertex_r(v0.v_id);
  EXPECT_EQ(v.v_id, v0.v_id);
  EXPECT_EQ(fc_id0, model->get_face_r(fc_id0).f_id);
}

TEST(he_mesh, add_face) {
  auto model = he_mesh::create();
  /*
   *  v0 - v2
   *   | /  |
   *  v1 - v3
   */
  auto v0 = vertex{ { 0.f, 0.f, 0.f }, 0 };
  auto v1 = vertex{ { 0.f, 1.f, 0.f }, 1 };
  auto v2 = vertex{ { 1.f, 0.f, 0.f }, 2 };
  auto v3 = vertex{ { 1.f, 1.f, 0.f }, 3 };
  auto fc_id0 = model->add_face(v0, v1, v2, 0);
  EXPECT_EQ(model->get_half_edge_count(), 3);
  EXPECT_EQ(model->get_face_count(), 1);
  EXPECT_EQ(model->get_vertex_count(), 3);
  // not necessary for model loading (model vertices already has its normal)
  EXPECT_EQ(model->get_face(fc_id0).normal, vec3d(0.f, 0.f, -1.f));
  auto fc_id1 = model->add_face(v1, v3, v2, 1);
  EXPECT_EQ(model->get_half_edge_count(), 6);
  EXPECT_EQ(model->get_face_count(), 2);
  EXPECT_EQ(model->get_vertex_count(), 4);
  auto v4 = vertex{ { 0.f, 0.f, 1.f }, 4 };
  auto fc_id2 = model->add_face(v2, v3, v4, 2);
//  EXPECT_TRUE(eps_eq(model->get_vertex_r(v2.v_id).normal, vec3(sqrt(2)/6.f, 0.f, (sqrt(2)/2.f - 2.f) / 3.f).normalized()));
  EXPECT_TRUE(eps_eq(model->get_face(fc_id2).normal, vec3d(sqrt(2)/2.f, 0.f, sqrt(2)/2.f)));
}

TEST(half_edge, pair) {
/*
 * - v0 - e2/ex - v2 - ex/ex - v4 - ex/ex - v0
 *    |         /  |         /  |         /
 *e0/ex  e1/e3 ex/e4  ex/ex ex/ex   ex/ex
 *    | /          |   /        | /
 * - v1 - ex/ex - v3 - ex/ex - v5 - ex/ex - v1
 */
  auto model = he_mesh::create();
  auto v0 = vertex{ { 0.f, 0.f, 0.f }, 0};
  auto v1 = vertex{ { 0.f, -1.f, 0.f}, 1};
  auto v2 = vertex{ { 1.f, 0.f, 0.f }, 2};
  auto v3 = vertex{ { 1.f, -1.f, 0.f}, 3};
  auto v4 = vertex{ { 0.f, 0.f, 1.f }, 4};
  auto v5 = vertex{ { 0.f, -1.f, 1.f}, 5};

  model->add_face(v0, v1, v2, 0);
  auto pair_id = model->get_half_edge_r(v1, v2).pair;
  EXPECT_EQ(model->exist_half_edge(pair_id), false);

  model->add_face(v1, v3, v2, 1);
  pair_id = model->get_half_edge_r(v1, v2).pair;
  EXPECT_EQ(model->get_half_edge_r(v1, v2).this_id, model->get_half_edge_r(pair_id).pair);

  model->add_face(v2, v3, v4, 2);
  model->add_face(v3, v5, v4, 3);
  model->add_face(v4, v5, v0, 4);
  model->add_face(v5, v1, v0, 5);
  pair_id = model->get_half_edge_r(v0, v1).pair;
  EXPECT_EQ(model->get_half_edge_r(v0, v1).this_id, model->get_half_edge_r(pair_id).pair);

  // invalid half-edge
  EXPECT_EQ(model->exist_half_edge(v0, v3), false);

  // unpaired half-edge
  pair_id = model->get_half_edge_r(v2, v0).pair;
  EXPECT_EQ(model->exist_half_edge(pair_id), false);
}


// perspective frustum -------------------------------------------------------------
bool operator== (const plane& rhs, const plane& lhs) {
  double eps = 0.0001;
  bool point_eq  = (std::abs(rhs.point.x() - lhs.point.x()) < eps) && (std::abs(rhs.point.y() - lhs.point.y()) < eps) && (std::abs(rhs.point.z() - lhs.point.z()) < eps);
  if (!point_eq) return false;
  bool normal_eq = (std::abs(rhs.normal.x() - lhs.normal.x()) < eps) && (std::abs(rhs.normal.y() - lhs.normal.y()) < eps) && (std::abs(rhs.normal.z() - lhs.normal.z()) < eps);
  return normal_eq;
}

TEST(perspective_frustum_test, ctor) {
  perspective_frustum frustum = {M_PI / 1.5f, M_PI / 1.5f, 3, 9};
  auto near   = frustum.get_near();
  auto far    = frustum.get_far();
  auto right  = frustum.get_right();
  auto left   = frustum.get_left();
  auto top    = frustum.get_top();
  auto bottom = frustum.get_bottom();
  plane my_near   = {vec3d(0.f, 0.f, 3.f), vec3d{0.f, 0.f, 1.f}};
  plane my_far    = {vec3d(0.f, 0.f, 9.f), vec3d{0.f, 0.f, -1.f}};
  plane my_right  = {vec3d(0.f, 0.f, 0.f), vec3d{-std::cos(M_PI / 3.f), 0.f, std::sin(M_PI / 3.f)}};
  plane my_left   = {vec3d(0.f, 0.f, 0.f), vec3d{std::cos(M_PI / 3.f), 0.f,  std::sin(M_PI / 3.f)}};
  plane my_top    = {vec3d(0.f, 0.f, 0.f), vec3d{0.f, std::cos(M_PI / 3.f),  std::sin(M_PI / 3.f)}};
  plane my_bottom = {vec3d(0.f, 0.f, 0.f), vec3d{0.f, -std::cos(M_PI / 3.f), std::sin(M_PI / 3.f)}};
  EXPECT_EQ(near,   my_near);
  EXPECT_EQ(far,    my_far);
  EXPECT_EQ(right,  my_right);
  EXPECT_EQ(left,   my_left);
  EXPECT_EQ(top,    my_top);
  EXPECT_EQ(bottom, my_bottom);
}

TEST(perspective_frustum_test, transform) {
  perspective_frustum frustum = {M_PI / 1.5f, M_PI / 1.5f, 3, 9};
  utils::transform tf;
  tf.translation = {3, 4, 8};
  tf.rotation    = {M_PI / 3.f, 0.f, 0.f};
  auto rotate_mat = tf.rotate_mat3();

  // update its transform
  frustum.update_planes(tf);

  auto near   = frustum.get_near();
  auto far    = frustum.get_far();
  auto right  = frustum.get_right();
  auto left   = frustum.get_left();
  auto top    = frustum.get_top();
  auto bottom = frustum.get_bottom();
  plane my_near   = {vec3d(3.f, 4 -3.f * std::sin(M_PI / 3.f), 8 + 1.5f), vec3d{0.f, -std::sin(M_PI / 3.f), std::cos(M_PI / 3.f)}};
  plane my_far    = {vec3d(3.f, 4 -9.f * std::sin(M_PI / 3.f), 8 + 4.5f), vec3d{0.f, std::sin(M_PI / 3.f), -0.5f}};
//  plane my_right  = {vec3(3, 4, 8), vec3{-std::cos(M_PI / 3.f), 0.f, std::sin(M_PI / 3.f)}};
//  plane my_left   = {vec3(3, 4, 8), vec3{std::cos(M_PI / 3.f), 0.f,  std::sin(M_PI / 3.f)}};
//  plane my_top    = {vec3(3, 4, 8), vec3{0.f, std::cos(M_PI / 3.f),  std::sin(M_PI / 3.f)}};
//  plane my_bottom = {vec3(3, 4, 8), vec3{0.f, -std::cos(M_PI / 3.f), std::sin(M_PI / 3.f)}};

  EXPECT_EQ(near,   my_near);
  EXPECT_EQ(far,    my_far);
//  EXPECT_EQ(right,  my_right);
//  EXPECT_EQ(left,   my_left);
//  EXPECT_EQ(top,    my_top);
//  EXPECT_EQ(bottom, my_bottom);
}
} // namespace hnll::geometry