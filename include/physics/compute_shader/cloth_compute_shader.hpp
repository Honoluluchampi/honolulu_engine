#pragma once

// hnll
#include <game/compute_shader.hpp>

namespace hnll {

namespace graphics {
  class buffer;
  class desc_sets;
  class desc_pool;
}

namespace physics {

DEFINE_COMPUTE_SHADER(cloth_compute_shader)
{
  public:
    DEFAULT_COMPUTE_SHADER_CTOR(cloth_compute_shader);

    void setup();

    void render(const utils::compute_frame_info &info);

  private:
    struct vertex
    {
      alignas(16) vec3 position;
      alignas(16) vec3 normal;
    };

    void setup_desc_sets();

    std::vector<u_ptr<graphics::buffer>> mesh_buffers_;
    u_ptr<graphics::desc_sets> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;
};

}} // namespace hnll::physics