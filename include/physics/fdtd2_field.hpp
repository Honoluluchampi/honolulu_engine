#pragma once

// hnll
#include <graphics/desc_set.hpp>
#include <graphics/image_resource.hpp>
#include <game/actor.hpp>
#include <utils/common_alias.hpp>

namespace hnll::physics {

struct fdtd_info {
  float x_len;
  float y_len;
  float sound_speed;
  float rho;
  int   pml_count;
};

DEFINE_PURE_ACTOR(fdtd2_field)
{
  public:
    static u_ptr<fdtd2_field> create(const fdtd_info& info);
    explicit fdtd2_field(const fdtd_info& info);
    ~fdtd2_field();

    void update_frame() { frame_index_ = frame_index_ == 0 ? 1 : 0; }

    // getter
    std::vector<VkDescriptorSet> get_frame_desc_sets();
    uint32_t get_field_id() const { return field_id_; }
    int get_x_grid()      const { return x_grid_; }
    int get_y_grid()      const { return y_grid_; }
    float get_x_len()     const { return x_len_; }
    float get_y_len()     const { return y_len_; }
    float get_v_fac()     const { return v_fac_;  }
    float get_p_fac()     const { return p_fac_; }
    int   get_pml_count() const { return pml_count_; }
    float get_dt()        const { return dt_; }
    float get_dx()        const { return dx_; }
    float get_duration()  const { return duration_; }
    int   get_update_per_frame() const { return update_per_frame_; }
    bool  is_ready()      const { return is_ready_; } // is constructed

    // setter
    void add_duration(float dt) { duration_ += dt; }
    void set_update_per_frame(int rep) { update_per_frame_ = rep; }
    void set_as_target(fdtd2_field* target) const;

    static const std::vector<graphics::binding_info> field_bindings;
    static const std::vector<graphics::binding_info> texture_bindings;

  private:
    void compute_constants();
    void setup_desc_sets(const fdtd_info& info);
    void setup_textures(const fdtd_info& info);

    graphics::device& device_;
    u_ptr<graphics::desc_sets> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;

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

    // frame_buffering
    int frame_count_ = 2;
    int frame_index_ = 0;

    uint32_t field_id_;

    float duration_ = 0.f;
    int update_per_frame_ = 0;
    bool  is_ready_ = false;
};
} // namespace hnll::physics