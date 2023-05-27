// hnll
#include <graphics/acceleration_structure.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

class triangle_app
{
  public:
    triangle_app()
    {
      window_ = std::make_unique<graphics::window>(960, 820, "triangle app");
      device_ = std::make_unique<graphics::device>(*window_, utils::rendering_type::RAY_TRACING);
      setup();
    }

    void setup()
    {
      load_model();
      create_blas();
      create_tlas();
    }

    void load_model()
    {

    }

    void create_blas()
    {
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address;
      VkDeviceOrHostAddressConstKHR index_buffer_device_address;

      auto device = device_->get_device();
      vertex_buffer_device_address.deviceAddress = get_device_address(device, model_->get_vertex_vk_buffer());
      index_buffer_device_address.deviceAddress  = get_device_address(device, model_->get_index_vk_buffer());
    }

    void create_tlas()
    {

    }

    void create_storage_image()
    {

    }

  private:
    // available features and properties
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties_;

    u_ptr<graphics::window> window_;
    u_ptr<graphics::device> device_;

    u_ptr<graphics::static_mesh> model_;
};

} // namespace hnll