// hnll
#include "../fdtd_horn.hpp"

// lib
#include <gtest/gtest.h>

#define EXPECT_NEQ(a, b) EXPECT_TRUE(neq(a, b))

namespace hnll {

bool neq(double a, double b, double eps = 0.00001)
{ return std::abs(a - b) < eps; }

TEST(fdtd_horn, ctor) {
  auto horn = fdtd_horn::create(
    0.01f,
    0.02f,
    1.1f,
    340.f,
    6,
    { 2, 1, 2 },
    { { 0.1f, 0.2f }, { 0.3f, 0.4f }, { 0.2f, 0.2f } });

  // physical constants
  EXPECT_EQ(0.01f, horn->get_dt());
  EXPECT_EQ(0.02f, horn->get_dx());
  EXPECT_EQ(1.1f, horn->get_rho());
  EXPECT_EQ(340.f, horn->get_c());
  EXPECT_EQ(6, horn->get_pml_count());
  EXPECT_EQ(3, horn->get_segment_count());

  // dimensions
  auto dim = horn->get_dimensions();
  auto size_infos = horn->get_size_infos();
  EXPECT_EQ(dim.size(), size_infos.size());
  EXPECT_EQ(2, dim[0]);
  EXPECT_EQ(1, dim[1]);
  EXPECT_EQ(2, dim[2]);
  EXPECT_NEQ(0.1f, size_infos[0].x());
  EXPECT_NEQ(0.2f, size_infos[0].y());
  EXPECT_NEQ(2.f,  size_infos[0].w());
  EXPECT_NEQ(0.3f, size_infos[1].x());
  EXPECT_NEQ(0.4f, size_infos[1].y());
  EXPECT_NEQ(1.f,  size_infos[1].w());
  EXPECT_NEQ(0.2f, size_infos[2].x());
  EXPECT_NEQ(0.2f, size_infos[2].y());
  EXPECT_NEQ(2.f,  size_infos[2].w());

  auto edge_infos = horn->get_edge_infos();
  EXPECT_NEQ(0.0f, edge_infos[0].x());
  EXPECT_NEQ(0.1f, edge_infos[1].x());
  EXPECT_NEQ(0.4f, edge_infos[2].x());
  EXPECT_NEQ(0.6f, edge_infos[3].x());
  EXPECT_NEQ(0.6f, horn->get_x_max());

  EXPECT_EQ(0, edge_infos[0].y());
  EXPECT_EQ(50, edge_infos[1].y());
  EXPECT_EQ(65, edge_infos[2].y());
  EXPECT_EQ(549, edge_infos[3].y());
  EXPECT_EQ(549, horn->get_whole_grid_count());

  auto grid_counts = horn->get_grid_counts();
  EXPECT_EQ(5,  grid_counts[0].x());
  EXPECT_EQ(10, grid_counts[0].y());
  EXPECT_EQ(15, grid_counts[1].x());
  EXPECT_EQ(1,  grid_counts[1].y());
  EXPECT_EQ(22, grid_counts[2].x());
  EXPECT_EQ(22, grid_counts[2].y());
}

TEST(fdtd_horn, pml_marking) {
  auto horn = fdtd_horn::create(
    1.f,
    1.f,
    1.1f,
    340.f,
    2,
    { 2 },
    { { 1.f, 1.f } });

  const auto& grid_conditions = horn->get_grid_conditions();

  for (int i = 0; i < grid_conditions.size(); i++) {
    if (i == 12)
      EXPECT_EQ(grid_conditions[i].x(), fdtd_horn::grid_type::NORMAL);
    else
      EXPECT_EQ(grid_conditions[i].x(), fdtd_horn::grid_type::PML);
  }
}

} // namespace hnll