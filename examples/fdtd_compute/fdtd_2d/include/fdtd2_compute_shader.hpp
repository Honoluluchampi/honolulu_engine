#pragma once

// hnll
#include <game/compute_shader.hpp>
#include <graphics/timeline_semaphore.hpp>

namespace hnll {

class fdtd2_field;

DEFINE_COMPUTE_SHADER(fdtd2_compute_shader)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(fdtd2_compute_shader);
    ~fdtd2_compute_shader(){}

    void setup() {}
    void render(const utils::compute_frame_info& info);

    static void set_target(fdtd2_field* target);
    static void remove_target(uint32_t target_id);

  private:
    static fdtd2_field* target_;
    static uint32_t target_id_;
};
} // namespace hnll