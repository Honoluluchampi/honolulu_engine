// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/ray_tracing_system.hpp>
#include <graphics/acceleration_structure.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <graphics/utils.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace hnll {

const std::string SHADERS_DIRECTORY =
  std::string(std::getenv("HNLL_ENGN")) + "/examples/ray_tracing/model/shaders/spv/";

#define MODEL_NAME utils::get_full_path("bunny.obj")

// to be ray_tracing_model
DEFINE_PURE_ACTOR(object)
{
  public:
    static u_ptr<object> create(graphics::device& device, std::string model_path)
    { return std::make_unique<object>(device, model_path); }

    object(graphics::device& device, std::string model_path) : device_(device)
    {
      mesh_model_ = graphics::static_mesh::create_from_file(device, model_path, true);
      vertex_count_ = mesh_model_->get_vertex_count();
      index_count_ = mesh_model_->get_face_count() * 3;

      create_blas();
    }

    void create_blas()
    {
      // blas build setup
      uint32_t num_triangles = index_count_ / 3;

      // get vertex buffer device address
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address {
        .deviceAddress = graphics::get_device_address(device_.get_device(), mesh_model_->get_vertex_vk_buffer())
      };
      VkDeviceOrHostAddressConstKHR index_buffer_device_address {
        .deviceAddress = graphics::get_device_address(device_.get_device(), mesh_model_->get_index_vk_buffer())
      };
      // geometry
      VkAccelerationStructureGeometryKHR as_geometry {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
      };
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      as_geometry.geometry.triangles.vertexData = vertex_buffer_device_address;
      as_geometry.geometry.triangles.maxVertex = vertex_count_;
      as_geometry.geometry.triangles.vertexStride = sizeof(graphics::vertex);
      as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
      as_geometry.geometry.triangles.indexData = index_buffer_device_address;

      // execute blas build command (in GPU)
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = num_triangles;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;

      graphics::acceleration_structure::input blas_input = {
        .geometry = { as_geometry },
        .build_range_info = { as_build_range_info },
      };

      blas_ = graphics::acceleration_structure::create(device_);
      blas_->build_as(
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        blas_input,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
      );
      blas_->destroy_scratch_buffer();
    }

  private:
    graphics::device& device_;
    u_ptr<graphics::static_mesh> mesh_model_;
    uint32_t vertex_count_;
    uint32_t index_count_;
    u_ptr<graphics::acceleration_structure> blas_;
};

// shading system
DEFINE_RAY_TRACER(model_ray_tracer, utils::shading_type::RAY1)
{
  public:
    model_ray_tracer(graphics::device& device) : game::ray_tracing_system<model_ray_tracer, utils::shading_type::RAY1>(device) {}

    void render(const utils::graphics_frame_info& frame_info)
    {
      set_current_command(frame_info.command_buffer);

      bind_pipeline();
      bind_desc_sets();
      dispatch_command();

    }

    void setup()
    {
      // tlas and vertex, index buffer layout
      // get desc image layout by gui::renderer
      auto scene_desc_layout = graphics::desc_layout::builder(device_)
        .add_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // tlas
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // vertex buffer
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // index buffer
        .build();

      auto& gui_engine = utils::singleton<game::gui_engine>::get_instance();

      auto vp_desc_layout = gui_engine.get_vp_image_desc_layout();

      sbt_ = graphics::shader_binding_table::create(
        device_,
        { scene_desc_layout->get_descriptor_set_layout(), vp_desc_layout },
        {
          SHADERS_DIRECTORY + "model.rgen.spv",
          SHADERS_DIRECTORY + "model.rmiss.spv",
          SHADERS_DIRECTORY + "shadow.rmiss.spv",
          SHADERS_DIRECTORY + "shadow.rchit.spv",
        },
        {
          VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          VK_SHADER_STAGE_MISS_BIT_KHR,
          VK_SHADER_STAGE_MISS_BIT_KHR,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
        }
      );

      // create blas
      mesh_model_ = object::create(device_, MODEL_NAME);
    }

  private:
    u_ptr<object> mesh_model_;
    std::vector<VkDescriptorSet> vp_desc_sets_;
};

SELECT_SHADING_SYSTEM(model_ray_tracer);
SELECT_COMPUTE_SHADER();
SELECT_ACTOR(object);

DEFINE_ENGINE(hello_model) {

};

} // namespace hnll

int main() {
  hnll::hello_model app {};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}