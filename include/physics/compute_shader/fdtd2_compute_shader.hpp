#pragma once

// hnll
#include <game/compute_shader.hpp>

namespace hnll::physics {

DEFINE_COMPUTE_SHADER(fdtd2_compute_shader)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(fdtd2_compute_shader);
    ~fdtd2_compute_shader();

    void setup();
    void render(const utils::compute_frame_info& info);

  private:
    u_ptr<graphics::desc_layout> layout_;
};
} // namespace hnll::physics