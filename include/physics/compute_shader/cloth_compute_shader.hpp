#pragma once

// hnll
#include <game/compute_shader.hpp>

namespace hnll {

namespace physics {

class mass_spring_cloth;

DEFINE_COMPUTE_SHADER(cloth_compute_shader)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(cloth_compute_shader);
    ~cloth_compute_shader();

    void setup();

    void render(const utils::compute_frame_info &info);

    // setter
    static void add_cloth(const s_ptr<mass_spring_cloth>& cloth);

  private:
    static std::unordered_map<uint32_t, s_ptr<mass_spring_cloth>> clothes_;
};

}} // namespace hnll::physics