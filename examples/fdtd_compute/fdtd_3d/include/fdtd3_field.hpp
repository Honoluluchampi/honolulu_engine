#pragma once

#include <graphics/desc_set.hpp>
#include <game/actor.hpp>
#include <utils/common_alias.hpp>

namespace hnll {

struct fdtd_info {
  vec3 length;
  float sound_speed;
  float rho;
  int pml_count;
  int update_per_frame;
};

DEFINE_PURE_ACTOR(fdtd3_field)
{
  public:
    static u_ptr<fdtd3_field> create(const fdtd_info& info);
    explicit fdtd3_field(const fdtd_info& info);
    ~fdtd3_field();

    void update_frame() { }

    // getter
    inline const vec3& get_length()     const { return length_; }
    inline float get_sound_speed()      const { return c_; }
    inline float get_rho()              const { return rho_; }
    inline int   get_pml_count()        const { return pml_count_; }
    inline int   get_update_per_frame() const { return update_per_frame_; }
    inline float get_dx() const { return dx_; }
    inline float get_dt() const { return dt_; }
    inline float get_v_fac() const { return v_fac_; }
    inline float get_p_fac() const { return p_fac_; }
    inline ivec3 get_grid_count() const { return grid_count_; }
    inline bool  get_is_ready() const { return is_ready_; }

  private:
    // init functions
    void compute_constants();
    void setup_desc_sets(const fdtd_info& info);

    graphics::device& device_; // as singleton
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;

    // physical constants
    float rho_;
    float c_; // sound speed
    vec3  length_;
    ivec3 grid_count_;
    float dt_, dx_;
    float p_fac_, v_fac_;
    float mouth_pressure_ = 0.f; // input

    // frame buffering
    int frame_count_ = 3;
    int frame_index_ = 0;

    size_t active_grid_count_;

    uint32_t field_id_;

    int pml_count_;

    float duration_ = 0.f;
    int update_per_frame_ = 0;
    bool is_ready_ = false; // for sound setup

    int listener_index_ = 0;
    // pointer for sound buffer
    float* sound_buffers_[3];

    static const std::vector<graphics::binding_info> field_bindings;
};

} // namespace hnll