// hnll
#include <game/compute_shader.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll {

DEFINE_COMPUTE_SHADER(cloth_compute_shader)
{
  public:
    void setup()
    {
      setup_desc_sets();
    }

  private:
    struct vertex { alignas(16) vec3 position; };

    void setup_desc_sets()
    {
      // pool

      // layout
      desc_sets_->add_layout(VK_SHADER_STAGE_COMPUTE_BIT);

      // assign buffer
      for (int i = 0; i < graphics::swap_chain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto new_buffer = graphics::buffer::create(
          device_,
          sizeof(vertex) * 3,
          1,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          nullptr
        );
        desc_sets_->add_buffer(std::move(new_buffer));
      }

      desc_sets_->build_sets();
    }

    std::vector<u_ptr<graphics::buffer>> mesh_buffers_;
    u_ptr<graphics::desc_set> desc_sets_;
};

}