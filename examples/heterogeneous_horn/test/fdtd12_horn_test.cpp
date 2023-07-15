// hnll
#include "../fdtd12_horn.hpp"

// lib
#include <gtest/gtest.h>

#define EXPECT_NEQ(a, b) EXPECT_TRUE(neq(a, b))

namespace hnll {

bool neq(double a, double b, double eps = 0.00001)
{ return std::abs(a - b) < eps; }

TEST(fdtd12_horn, ctor)
{
  auto horn = fdtd_horn::create(
    1.f,
    1.f,
    1.1f,
    340.f,
    1,
    0.5,
    {2, 1, 2},
    {{3.f, 4.f},
     {3.f, 2.f},
     {3.f, 4.f}});

  // physical constants
  EXPECT_EQ(1.f, horn->get_dt());
  EXPECT_EQ(1.f, horn->get_dx());
  EXPECT_EQ(1.1f, horn->get_rho());
  EXPECT_EQ(340.f, horn->get_c());
  EXPECT_EQ(1, horn->get_pml_count());
  EXPECT_EQ(3, horn->get_segment_count());

  // grid configuration
  EXPECT_EQ(10, horn->get_whole_x());
  EXPECT_EQ(5, horn->get_whole_y());

  const auto& grid_conditions = horn->get_grid_conditions();
  int test_grid_types[] = {
    0, 0, 0, 0, 0, 5, 5, 5, 5, 5,
    2, 2, 2, 3, 3, 3, 2, 2, 2, 5,
    2, 2, 2, 1, 1, 1, 2, 2, 2, 5,
    2, 2, 2, 3, 3, 3, 2, 2, 2, 5,
    0, 0, 0, 0, 0, 5, 5, 5, 5, 5,
  };

  for (int i = 0; i < 27; i++) {
    std::cout << i << std::endl;
    EXPECT_EQ(grid_conditions[i].x(), test_grid_types[i]);
  }
}

} // namespace hnll