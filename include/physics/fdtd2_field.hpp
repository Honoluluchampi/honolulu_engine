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
    ~fdtd2_field();

    void update_frame() { frame_index_ = frame_index_ == 0 ? 1 : 0; }

    // getter
    std::vector<VkDescriptorSet> get_frame_desc_sets();
    uint32_t get_field_id() const { return field_id_; }
    int get_x_grid() const { return x_grid_; }
    int get_y_grid() const { return y_grid_; }
    float get_x_len() const { return x_len_; }
    float get_y_len() const { return y_len_; }
    float get_v_fac() const { return 1 / (rho_ * grid_size_);  }
    float get_p_fac() const { return kappa_ / grid_size_; }
    float get_f_max() const { return f_max_; }
    int get_restart() const { return restart_; }
    float get_duration() const { return duration_; }

    // setter
    void set_restart(int state) { restart_ = state; }
    void reset_duration() { duration_ = 0.f; }
    void add_duration(float dt) { duration_ += dt; }

    static const std::vector<graphics::binding_info> field_bindings;

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

    uint32_t field_id_;

    int restart_ = 2;
    float duration_ = 0.f;
};
} // namespace hnll::physics