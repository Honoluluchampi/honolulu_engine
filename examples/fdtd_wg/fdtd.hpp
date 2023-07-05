#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll {

struct fdtd1;

struct fdtd2
{
  fdtd2() = default;
  void build(
    float w,
    float h,
    int pml_layer_count = 20,
    bool up_pml = true,
    bool down_pml = true,
    bool left_pml = true,
    bool right_pml = true);

  void update();

  void get_field();

  std::vector<float> p;
  std::vector<float> vx;
  std::vector<float> vy;

  ivec2 main_grid_count; // grid count of main region
  ivec2 grid_count;
  int whole_grid_count;
  int pml_count;
  float height;
  float width;
  bool pml_up;
  bool pml_down;
  bool pml_left;
  bool pml_right;
};

struct fdtd3;

} // namespace hnll