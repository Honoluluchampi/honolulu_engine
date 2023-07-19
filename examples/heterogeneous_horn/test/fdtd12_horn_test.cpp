// hnll
#include "../fdtd12_horn.hpp"

// lib
#include <gtest/gtest.h>

#define EXPECT_NEQ(a, b) EXPECT_TRUE(neq(a, b))

namespace hnll {

bool neq(double a, double b, double eps = 0.00001)
{ return std::abs(a - b) < eps; }

bool operator==(const std::vector<int>& a, const std::vector<int>& b)
{
  if (a.size() != b.size())
    return false;

  for (int i = 0; i < a.size(); i++) {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

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
    3, 3, 3, 3, 3, 3, 2, 2, 2, 5,
    4, 2, 9, 6, 1, 7, 8, 2, 2, 5,
    3, 3, 3, 3, 3, 3, 2, 2, 2, 5,
    0, 0, 0, 0, 0, 5, 5, 5, 5, 5,
  };

  for (int i = 0; i < 50; i++) {
    std::cout << i << std::endl;
    EXPECT_EQ(grid_conditions[i].x(), test_grid_types[i]);
  }

  // check gathered indices
  std::vector<int> test_1d = { 24 };
  std::vector<int> test_2d = { 16, 17, 18, 21, 27, 28, 36, 37, 38 };
  std::vector<int> test_pml = { 5, 6, 7, 8, 9, 19, 29, 39, 45, 46, 47, 48, 49 };
  std::vector<int> test_exc = { 20 };
  std::vector<int> test_j12l = { 23 };
  std::vector<int> test_j12r = { 25 };
  std::vector<int> test_j21l = { 26 };
  std::vector<int> test_j21r = { 22 };

  EXPECT_TRUE(horn->get_ids_1d() == test_1d);
  EXPECT_TRUE(horn->get_ids_2d() == test_2d);
  EXPECT_TRUE(horn->get_ids_pml() == test_pml);
  EXPECT_TRUE(horn->get_ids_exc() == test_exc);
  EXPECT_TRUE(horn->get_ids_j12l() == test_j12l);
  EXPECT_TRUE(horn->get_ids_j12r() == test_j12r);
  EXPECT_TRUE(horn->get_ids_j21l() == test_j21l);
  EXPECT_TRUE(horn->get_ids_j21r() == test_j21r);
}

} // namespace hnll