#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll {

namespace graphics {
  class device;
  class desc_sets;
  class desc_pool;
}

namespace physics {

struct fdtd_info {
  float x_len;
  float y_len;
  float sound_speed;
  float kappa;
  float rho;
  float f_max;
};

class fdtd2_field
{
  public:
    u_ptr<fdtd2_field> create(const fdtd_info& info);
    explicit fdtd2_field(const fdtd_info& info);

  private:
    void compute_constants();
    void setup_desc_sets();

    graphics::device& device_;
    u_ptr<graphics::desc_sets> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;

    // physical constants
    float kappa_;
    float rho_;
    float f_max_;
    float sound_speed_;

    float x_len_, y_len_;
    int x_grid_, y_grid_;
    float dt_, grid_size_;
};
}} // namespace hnll::physics