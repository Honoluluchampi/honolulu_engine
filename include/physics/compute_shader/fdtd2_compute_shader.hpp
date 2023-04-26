#pragma once

// hnll
#include <game/compute_shader.hpp>

namespace hnll::physics {

class fdtd2_field;

DEFINE_COMPUTE_SHADER(fdtd2_compute_shader)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(fdtd2_compute_shader);
    ~fdtd2_compute_shader();

    void setup();
    void render(const utils::compute_frame_info& info);

    static void set_target(const s_ptr<fdtd2_field>& target);
    static void remove_target(uint32_t field_id);

  private:
    u_ptr<graphics::desc_layout> layout_;
    static s_ptr<fdtd2_field> target_;
};
} // namespace hnll::physics