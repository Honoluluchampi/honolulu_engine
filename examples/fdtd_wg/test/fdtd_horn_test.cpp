// hnll
#include "../fdtd_horn.hpp"

// lib
#include <gtest/gtest.h>

namespace hnll {

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

  // dimensions
  auto dim = horn->get_dimensions();
  auto size_infos = horn->get_size_infos();
  EXPECT_EQ(dim.size(), size_infos.size());
  EXPECT_EQ(2, dim[0]);
  EXPECT_EQ(1, dim[1]);
  EXPECT_EQ(2, dim[2]);
  EXPECT_DOUBLE_EQ(0.1f, size_infos[0].x());
  EXPECT_DOUBLE_EQ(0.2f, size_infos[0].y());
  EXPECT_DOUBLE_EQ(0.1f, size_infos[0].z());
  EXPECT_DOUBLE_EQ(2.f,  size_infos[0].w());
  EXPECT_DOUBLE_EQ(0.3f, size_infos[1].x());
  EXPECT_DOUBLE_EQ(0.4f, size_infos[1].y());
  EXPECT_DOUBLE_EQ(0.4f, size_infos[1].z());
  EXPECT_DOUBLE_EQ(1.f,  size_infos[1].w());
  EXPECT_DOUBLE_EQ(0.2f, size_infos[2].x());
  EXPECT_DOUBLE_EQ(0.2f, size_infos[2].y());
  EXPECT_DOUBLE_EQ(0.6f, size_infos[2].z());
  EXPECT_DOUBLE_EQ(2.f,  size_infos[2].w());

  auto start_grid_ids = horn->get_start_grid_ids();
  EXPECT_EQ(0, start_grid_ids[0]);
  EXPECT_EQ(50, start_grid_ids[1]);
  EXPECT_EQ(65, start_grid_ids[2]);
  EXPECT_EQ(549, start_grid_ids[3]);
  EXPECT_EQ(549, horn->get_whole_grid_count());

  auto grid_counts = horn->get_grid_counts();
  EXPECT_EQ(5,  grid_counts[0].x());
  EXPECT_EQ(10, grid_counts[0].y());
  EXPECT_EQ(15, grid_counts[1].x());
  EXPECT_EQ(1,  grid_counts[1].y());
  EXPECT_EQ(22, grid_counts[2].x());
  EXPECT_EQ(22, grid_counts[2].y());
}

} // namespace hnll