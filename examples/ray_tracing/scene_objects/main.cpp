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
  std::string(std::getenv("HNLL_ENGN")) + "/examples/ray_tracing/scene_objects/shaders/spv/";

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

    const graphics::acceleration_structure& get_blas() const { return *blas_; }
    VkDescriptorBufferInfo get_vertex_buffer_info() const { return mesh_model_->get_vertex_buffer_info(); }
    VkDescriptorBufferInfo get_index_buffer_info() const { return mesh_model_->get_index_buffer_info(); }

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
    ~model_ray_tracer() {}

    void render(const utils::graphics_frame_info& frame_info)
    {
      set_current_command(frame_info.command_buffer);

      bind_pipeline();
      bind_desc_sets({scene_desc_set_, vp_desc_sets_[frame_info.frame_index]});
      dispatch_command(
        static_cast<uint32_t>(960 * (1.f - 0.2f)),
        static_cast<uint32_t>(820 * (1.f - 0.25f)),
        1
      );
    }

    void setup()
    {
      // acceleration structure ----------------------------------------------------------
      // create blas
      mesh_model_ = object::create(device_, MODEL_NAME);

      setup_tlas();

      // desc sets -----------------------------------------------------------------------
      auto& gui_engine = utils::singleton<game::gui_engine>::get_instance();

      // setup layout
      // tlas and vertex, index buffer layout
      // get desc image layout by gui::renderer
      auto scene_desc_layout = graphics::desc_layout::builder(device_)
        .add_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // tlas
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // vertex buffer
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // index buffer
        .build();
      auto vp_desc_layout = gui_engine.get_vp_image_desc_layout();
      vp_desc_sets_ = gui_engine.get_vp_image_desc_sets();

      // setup desc sets
      scene_desc_pool_ = graphics::desc_pool::builder(device_)
        .set_max_sets(10)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000)
        .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .build();

      VkWriteDescriptorSetAccelerationStructureKHR as_info = tlas_->get_as_info();

      auto vertex_buffer_info = mesh_model_->get_vertex_buffer_info();
      auto index_buffer_info  = mesh_model_->get_index_buffer_info();

      graphics::desc_writer(*scene_desc_layout, *scene_desc_pool_)
        .write_acceleration_structure(0, &as_info)
        .write_buffer(1, &vertex_buffer_info)
        .write_buffer(2, &index_buffer_info)
        .build(scene_desc_set_);

      // shader binding table ------------------------------------------------------
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
    }

    void setup_tlas()
    {
      VkTransformMatrixKHR transform_matrix = {
        0.3f, 0.f, 0.f, 0.f,
        0.f, 0.3f, 0.f, 0.f,
        0.f, 0.f, 0.3f, 0.f
      };

      VkAccelerationStructureInstanceKHR as_instance {};
      as_instance.transform = transform_matrix;
      as_instance.instanceCustomIndex = 0;
      as_instance.mask = 0xFF;
      as_instance.instanceShaderBindingTableRecordOffset = 0;
      as_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      as_instance.accelerationStructureReference = mesh_model_->get_blas().get_as_device_address();

      VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
      VkMemoryPropertyFlags host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      VkDeviceSize buffer_size = sizeof(VkAccelerationStructureInstanceKHR);

      // create instances buffer
      instances_buffer_ = std::make_unique<graphics::buffer>(
        device_,
        buffer_size,
        1,
        usage,
        host_memory_props
      );

      // write the data to the buffer
      instances_buffer_->map();
      instances_buffer_->write_to_buffer((void *) &as_instance);

      // compute required memory size
      VkDeviceOrHostAddressConstKHR instance_data_device_address {
        .deviceAddress = graphics::get_device_address(device_.get_device(), instances_buffer_->get_buffer())
      };

      VkAccelerationStructureGeometryKHR as_geometry {};
      as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
      as_geometry.geometry.instances.data = instance_data_device_address;

      // count of objects
      uint32_t primitive_count = 1;
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = primitive_count;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;

      // build
      graphics::acceleration_structure::input tlas_input {};
      tlas_input.geometry = { as_geometry };
      tlas_input.build_range_info = { as_build_range_info };

      tlas_ = graphics::acceleration_structure::create(device_);
      tlas_->build_as(
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        tlas_input,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
      );
      tlas_->destroy_scratch_buffer();
    }

  private:
    u_ptr<object> mesh_model_;
    u_ptr<graphics::acceleration_structure> tlas_;
    u_ptr<graphics::buffer> instances_buffer_;
    std::vector<VkDescriptorSet> vp_desc_sets_;
    VkDescriptorSet scene_desc_set_;
    s_ptr<graphics::desc_pool> scene_desc_pool_;
};

SELECT_SHADING_SYSTEM(model_ray_tracer);
SELECT_COMPUTE_SHADER();
SELECT_ACTOR(object);

DEFINE_ENGINE(hello_model) {
  public:
    ENGINE_CTOR(hello_model) {}
};

} // namespace hnll

int main() {
  hnll::hello_model app {"scene_objects", hnll::utils::rendering_type::RAY_TRACING};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}