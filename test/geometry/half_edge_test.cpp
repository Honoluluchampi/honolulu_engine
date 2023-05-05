// hnll
#include <geometry/primitives.hpp>
#include <geometry/mesh_model.hpp>

// lib
#include <gtest/gtest.h>

const double EPSILON = 0.0001f;

namespace hnll::geometry {

bool eps_eq(const vec3& a, const vec3& b)
{
  return (abs(a.x()-b.x()) < EPSILON && abs(a.y()-b.y()) < EPSILON && abs(a.z()-b.z()) < EPSILON);
}

TEST(vertex, ctor) {
  auto v = vertex::create({0.f, 1.f, 0.f});
  EXPECT_EQ(v->position_, vec3(0.f, 1.f, 0.f));
  auto he = half_edge::create(v);
  EXPECT_EQ(v->half_edge_, he);
  auto v1 = vertex::create({0.f, 0.f, 0.f});
}

TEST(common, id) {
  auto model = mesh_model::create();
  /*
   *  v0 - v2
   *   | /  |
   *  v1 - v3
   */
  auto v0 = vertex::create({ 0.f, 0.f, 0.f });
  auto v1 = vertex::create({ 0.f, 1.f, 0.f});
  auto v2 = vertex::create({ 1.f, 0.f, 0.f });
  auto v3 = vertex::create({ 1.f, 1.f, 0.f});
  auto fc_id0 = model->add_face(v0, v1, v2);
  auto v = model->get_vertex(v0->id_);
  EXPECT_EQ(v->id_, v0->id_);
  EXPECT_EQ(fc_id0, model->get_face(fc_id0)->id_);
}

TEST(mesh_model, add_face) {
  auto model = mesh_model::create();
  /*
   *  v0 - v2
   *   | /  |
   *  v1 - v3
   */
  auto v0 = vertex::create({ 0.f, 0.f, 0.f }, 0);
  auto v1 = vertex::create({ 0.f, 1.f, 0.f}, 1);
  auto v2 = vertex::create({ 1.f, 0.f, 0.f }, 2);
  auto v3 = vertex::create({ 1.f, 1.f, 0.f}, 3);
  auto fc_id0 = model->add_face(v0, v1, v2, hnll::geometry::auto_vertex_normal_calculation::ON);
  EXPECT_EQ(model->get_half_edge_count(), 3);
  EXPECT_EQ(model->get_face_count(), 1);
  EXPECT_EQ(model->get_vertex_count(), 3);
  EXPECT_EQ(model->get_vertex(v0->id_)->normal_, vec3(0.f, 0.f, -1.f));
  EXPECT_EQ(model->get_face(fc_id0)->normal_, vec3(0.f, 0.f, -1.f));
  auto fc_id1 = model->add_face(v1, v3, v2);
  EXPECT_EQ(model->get_half_edge_count(), 6);
  EXPECT_EQ(model->get_face_count(), 2);
  EXPECT_EQ(model->get_vertex_count(), 4);
  auto v4 = vertex::create({ 0.f, 0.f, 1.f });
  auto fc_id2 = model->add_face(v2, v3, v4, hnll::geometry::auto_vertex_normal_calculation::ON);
  EXPECT_TRUE(eps_eq(
    model->get_vertex(v2->id_)->normal_,
    vec3(sqrt(2)/6.f, 0.f, (sqrt(2)/2.f - 2.f) / 3.f).normalized()));
  EXPECT_TRUE(eps_eq(
    model->get_face(fc_id2)->normal_,
    vec3(sqrt(2)/2.f, 0.f, sqrt(2)/2.f)));
}

TEST(half_edge, pair) {
/*
 * - v0 - e2/ex - v2 - ex/ex - v4 - ex/ex - v0
 *    |         /  |         /  |         /
 *e0/ex  e1/e3 ex/e4  ex/ex ex/ex   ex/ex
 *    | /          |   /        | /
 * - v1 - ex/ex - v3 - ex/ex - v5 - ex/ex - v1
 */
  auto model = mesh_model::create();
  auto v0 = vertex::create({ 0.f, 0.f, 0.f });
  auto v1 = vertex::create({ 0.f, -1.f, 0.f});
  auto v2 = vertex::create({ 1.f, 0.f, 0.f });
  auto v3 = vertex::create({ 1.f, -1.f, 0.f});
  auto v4 = vertex::create({ 0.f, 0.f, 1.f });
  auto v5 = vertex::create({ 0.f, -1.f, 1.f});

  model->add_face(v0, v1, v2);
  EXPECT_EQ(model->get_half_edge(v1, v2)->get_pair(), nullptr);
  model->add_face(v1, v3, v2);
  EXPECT_EQ(model->get_half_edge(v1, v2), model->get_half_edge(v1, v2)->get_pair()->get_pair());
  model->add_face(v2, v3, v4);
  model->add_face(v3, v5, v4);
  model->add_face(v4, v5, v0);
  model->add_face(v5, v1, v0);
  EXPECT_EQ(model->get_half_edge(v0, v1), model->get_half_edge(v0, v1)->get_pair()->get_pair());
  // invalid half-edge
  EXPECT_EQ(model->get_half_edge(v0, v3), nullptr);
  // unpaired half-edge
  EXPECT_EQ(model->get_half_edge(v2, v0)->get_pair(), nullptr);
}
} // namespace hnll::geometry