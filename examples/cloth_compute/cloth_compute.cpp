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
      create_pipeline(
        utils::get_engine_root_path() + "/examples/cloth_compute/cloth_compute.spv",
        { desc_sets_->get_layout() }
      );
    }

  private:
    struct vertex { alignas(16) vec3 position; };

    void setup_desc_sets()
    {
      int frame_in_flight = graphics::swap_chain::MAX_FRAMES_IN_FLIGHT;
      // pool
      desc_pool_ = graphics::desc_pool::builder(device_)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_in_flight)
        .build();

      // build desc sets
      desc_sets_ = graphics::desc_set::create(device_, desc_pool_);
      desc_sets_->add_binding(VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_in_flight);

      // assign buffer
      for (int i = 0; i < frame_in_flight; i++) {
        auto new_buffer = graphics::buffer::create(
          device_,
          sizeof(vertex) * 3,
          1,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          nullptr
        );
        desc_sets_->set_buffer(0, i, std::move(new_buffer));
      }

      desc_sets_->build_sets();
    }

    std::vector<u_ptr<graphics::buffer>> mesh_buffers_;
    u_ptr<graphics::desc_set> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;
};

}