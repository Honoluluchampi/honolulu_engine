#pragma once

// hnll
#include <graphics/desc_set.hpp>
#include <graphics/image_resource.hpp>
#include <game/actor.hpp>
#include <utils/common_alias.hpp>

// std
#include <set>

namespace hnll::physics {

struct fdtd_info {
  float x_len;
  float y_len;
  float sound_speed;
  float rho;
  int   pml_count;
  int   update_per_frame;
};

DEFINE_PURE_ACTOR(fdtd2_field)
{
  public:
    static u_ptr<fdtd2_field> create(const fdtd_info& info);
    explicit fdtd2_field(const fdtd_info& info);
    ~fdtd2_field();

    void update_frame() { frame_index_ = frame_index_ == frame_count_ - 1 ? 0 : frame_index_ + 1; }

    // getter
    std::vector<VkDescriptorSet> get_frame_desc_sets(int game_frame_index);
    uint32_t get_field_id() const { return field_id_; }

    // push constant
    int get_x_grid()      const { return x_grid_; }
    int get_y_grid()      const { return y_grid_; }
    float get_x_len()     const { return x_len_; }
    float get_y_len()     const { return y_len_; }
    float get_v_fac()     const { return v_fac_;  }
    float get_p_fac()     const { return p_fac_; }
    float get_mouth_pressure() const { return mouth_pressure_; }

    size_t get_active_ids_count() const { return active_ids_count_; }
    int   get_pml_count() const { return pml_count_; }
    float get_dt()        const { return dt_; }
    float get_dx()        const { return dx_; }
    float get_duration()  const { return duration_; }
    bool  is_ready()      const { return is_ready_; } // is constructed
    int   get_update_per_frame() const { return update_per_frame_; }
    int   get_listener_index() const { return listener_index_; }
    float* get_sound_buffer(int game_frame_index) { return sound_buffers_[game_frame_index]; }

    // setter
    void add_duration() { duration_ += dt_ * update_per_frame_; }
    void set_update_per_frame(int rep) { update_per_frame_ = rep; }
    void set_as_target(fdtd2_field* target) const;
    void set_mouth_pressure(float mp) { mouth_pressure_ = mp; }

    static const std::vector<graphics::binding_info> field_bindings;
    static const std::vector<graphics::binding_info> texture_bindings;

  private:
    void compute_constants();
    void setup_desc_sets(const fdtd_info& info);
    void setup_textures(const fdtd_info& info);
    void set_pml(
      std::vector<vec4>& grids,
      std::set<int>&  active_ids,
      int x_min, int x_max, int y_min, int y_max);

    graphics::device& device_;
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;

    // texture
    std::vector<VkDescriptorSet> texture_vk_desc_sets_;

    // physical constants
    float rho_;
    float c_;

    float x_len_, y_len_;
    int x_grid_, y_grid_;
    float dt_, dx_;

    int pml_count_;

    float p_fac_;
    float v_fac_;

    float mouth_pressure_ = 0.f;

    // frame_buffering
    int frame_count_ = 3;
    int frame_index_ = 0;

    size_t active_ids_count_;

    uint32_t field_id_;

    float duration_ = 0.f;
    int update_per_frame_ = 0;
    bool  is_ready_ = false;

    int listener_index_ = 0;
    // pointer for sound buffer
    float* sound_buffers_[3];
};
} // namespace hnll::physics