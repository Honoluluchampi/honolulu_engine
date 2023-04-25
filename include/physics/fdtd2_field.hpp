#pragma once

// hnll
#include <graphics/desc_set.hpp>
#include <utils/common_alias.hpp>

namespace hnll::physics {

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
    static u_ptr<fdtd2_field> create(const fdtd_info& info);
    explicit fdtd2_field(const fdtd_info& info);

    std::vector<VkDescriptorSet> get_frame_desc_sets();

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

    // frame_buffering
    int frame_count_ = 2;
    int frame_index_ = 0;
};
} // namespace hnll::physics