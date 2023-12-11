#pragma once

// hnll
#include <graphics/desc_set.hpp>
#include <graphics/image_resource.hpp>
#include <game/actor.hpp>
#include <utils/common_alias.hpp>
#include <utils/singleton.hpp>

// std
#include <set>

namespace hnll {

struct fdtd_info {
  float z_len;
  float r_len;
  float sound_speed;
  float rho;
  int   pml_count;
  int   update_per_frame;
};

struct particle;

DEFINE_PURE_ACTOR(fdtd_cylindrical_field)
{
  public:
    static u_ptr<fdtd_cylindrical_field> create(const fdtd_info& info);
    explicit fdtd_cylindrical_field(const fdtd_info& info);
    ~fdtd_cylindrical_field();

    void update_frame() { frame_index_ = frame_index_ == frame_count_ - 1 ? 0 : frame_index_ + 1; }
    void update_sound_frame() { sound_frame_index_ = sound_frame_index_ == frame_count_ - 1 ? 0 : sound_frame_index_ + 1; }
    // getter
    std::vector<VkDescriptorSet> get_frame_desc_sets();
    uint32_t get_field_id() const { return field_id_; }

    // push constant
    int get_z_grid_count() const { return z_grid_count_; }
    int get_r_grid_count() const { return r_grid_count_; }
    int get_whole_grid_count() const { return whole_grid_count_; }
    float get_z_len() const { return z_len_; }
    float get_r_len() const { return r_len_; }
    float get_v_fac() const { return v_fac_;  }
    float get_p_fac() const { return p_fac_; }
    float get_mouth_pressure() const { return mouth_pressure_; }
    // returns -1 if fcm is off
    float get_fcm_freq() const { return fcm_on_ ? fcm_freq_ : -1; }
    int get_fcm_source_grid_id() const { return fcm_on_ ? fcm_source_grid_id_ : -1; }

    int   get_pml_count() const { return pml_count_; }
    float get_dt()        const { return dt_; }
    float get_dz()        const { return dz_; }
    float get_dr()        const { return dr_; }
    float get_duration()  const { return duration_; }
    bool  is_ready()      const { return is_ready_; } // is constructed
    int   get_update_per_frame() const { return update_per_frame_; }
    int   get_listener_index() const { return listener_index_; }
    float* get_sound_buffer() { return sound_buffers_[sound_frame_index_]; }

    // setter
    void add_duration() { duration_ += dt_ * update_per_frame_; }
    void set_update_per_frame(int rep) { update_per_frame_ = rep; }
    void set_as_target(fdtd_cylindrical_field* target) const;
    void set_mouth_pressure(float mp) { mouth_pressure_ = mp; }

    // frequency characteristic measurement
    void start_fcm() { fcm_on_ = true; }
    void end_fcm() { fcm_on_ = false; }
    void set_fcm_freq(float freq) { fcm_freq_ = freq; }

    static const std::vector<graphics::binding_info> field_bindings;

    // for debug
    std::vector<double> compute_input_impedance() const;

    // float func(float x)
    void set_bore_shape(
      std::vector<particle>& initial_grid,
      const std::function<float(float)>& func,
      float max_length);

  private:
    void compute_constants();
    void setup_desc_sets(const fdtd_info& info);
    void set_pml(std::vector<particle>& grids, int z_min, int z_max, int r_max);

    utils::single_ptr<graphics::device> device_;
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;

    // texture
    std::vector<VkDescriptorSet> texture_vk_desc_sets_;

    // physical constants
    float rho_;
    float c_;

    float z_len_, r_len_;
    int z_grid_count_, r_grid_count_, whole_grid_count_;
    float dt_, dz_, dr_;

    int pml_count_;

    float p_fac_;
    float v_fac_;

    float mouth_pressure_ = 0.f;

    // frame_buffering
    int frame_count_ = 3;
    int frame_index_ = 0;

    size_t exciter_grid_count_ = 0;

    uint32_t field_id_;

    float duration_ = 0.f;
    int update_per_frame_ = 0;
    bool  is_ready_ = false;

    int listener_index_ = 0;
    // pointer for sound buffer
    float* sound_buffers_[3];
    int sound_frame_index_ = 0;

    // fcm : frequency characteristic measurement
    bool fcm_on_ = false;
    float fcm_freq_ = 0;
    int fcm_source_grid_id_ = 0;
};
} // namespace hnll