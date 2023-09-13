// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/acceleration_structure.hpp>
#include <graphics/utils.hpp>
#include <utils/vulkan_config.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace hnll {

const std::string SHADERS_DIRECTORY =
  std::string(std::getenv("HNLL_ENGN")) + "/examples/ray_tracing/simple_triangle/shaders/spv/";

enum class shader_stages {
  RAY_GENERATION,
  MISS,
  CLOSEST_HIT,
  ANY_HIT,
  INTERSECTION,
  MAX_STAGE,
};

namespace scene_hit_shader_group {
const uint32_t plane_hit_shader = 0;
const uint32_t cube_hit_shader  = 1;
}

struct acceleration_structure
{
  VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
  VkDeviceMemory             memory = VK_NULL_HANDLE;
  VkBuffer                   buffer = VK_NULL_HANDLE;
  VkDeviceAddress            device_address = 0;
};

struct vertex
{
  vec3 position;
  vec3 normal;
  vec4 color;
};

struct ray_tracing_scratch_buffer
{
  VkBuffer        handle = VK_NULL_HANDLE;
  VkDeviceMemory  memory = VK_NULL_HANDLE;
  VkDeviceAddress device_address = 0;
};

template<class T> T align(T size, uint32_t align)
{ return (size + align - 1) & ~static_cast<T>(align - 1); }

class hello_model {
  public:
    hello_model()
    {
      window_ = std::make_unique<graphics::window>(960, 820, "hello ray tracing triangle");
      device_ = std::make_unique<graphics::device>(
        utils::rendering_type::RAY_TRACING
      );

      create_triangle_as();
    }

    void cleanup()
    {
      auto device = device_->get_device();
      destroy_acceleration_structure(*blas_);
      destroy_acceleration_structure(*tlas_);
      vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
      vkDestroyPipeline(device, pipeline_, nullptr);
      vkDestroySemaphore(device, render_completed_, nullptr);
      vkDestroySemaphore(device, present_completed_, nullptr);
      for (auto& cb : command_buffers_) {
        vkDestroyFence(device, cb->fence, nullptr);
      }
      for (auto& rt : render_targets_) {
        rt->set_is_for_swap_chain(true);
        rt.reset();
      }
      vkDestroySwapchainKHR(device, swap_chain_, nullptr);
      vkDestroySurfaceKHR(device_->get_instance(), surface_, nullptr);
    }

    void run()
    {
      while (glfwWindowShouldClose(window_->get_glfw_window()) == GLFW_FALSE) {
        glfwPollEvents();
        update();
        render();
      }
      vkDeviceWaitIdle(device_->get_device());
      cleanup();
    }

  private:

    void update()
    {

    }

    void render()
    {
      // get current available frame index
      wait_available_frame(); // renderer::begin_frame
      auto command = command_buffers_[frame_index_]->command;

      VkCommandBufferBeginInfo begin_info {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr, 0, nullptr
      };

      vkBeginCommandBuffer(command, &begin_info);
      // renderer::begin_frame

      auto extent = window_->get_extent();

      vkCmdBindPipeline(
        command,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipeline_
      );
      vkCmdBindDescriptorSets(
        command,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipeline_layout_,
        0,
        1,
        &desc_set_,
        0,
        nullptr
      );

      VkStridedDeviceAddressRegionKHR callable_shader_entry {};
      vkCmdTraceRaysKHR(
        command,
        &region_raygen_,
        &region_miss_,
        &region_hit_,
        &callable_shader_entry,
        extent.width, extent.height, 1
      );

      auto& back_buffer = render_targets_[frame_index_];
      VkImageCopy region{};
      region.extent = { extent.width, extent.height, 1 };
      region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

      ray_traced_image_->transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, command);
      back_buffer->transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command);

      vkCmdCopyImage(
        command,
        ray_traced_image_->get_image(),
        ray_traced_image_->get_image_layout(),
        back_buffer->get_image(),
        back_buffer->get_image_layout(),
        1,
        &region
      );

      ray_traced_image_->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL, command);
      back_buffer->transition_image_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, command);

      vkEndCommandBuffer(command);

      submit_current_frame_command_buffer();
      present();
    }

    void create_triangle_as()
    {
      // temporary swap chain
      create_swap_chain();
      create_vertex_buffer();
      create_triangle_blas();
      create_triangle_tlas();
      create_ray_traced_image();
      create_layout();
      create_pipeline();
      create_shader_binding_table();
      create_descriptor_set();
    }

    void create_vertex_buffer()
    {
      uint32_t vertex_count = triangle_vertices_.size();
      uint32_t vertex_size = sizeof(triangle_vertices_[0]);
      VkDeviceSize buffer_size = vertex_size * vertex_count;

      VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

      vertex_buffer_ = graphics::buffer::create_with_staging(
        *device_,
        buffer_size,
        1,
        usage,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        triangle_vertices_.data()
      );
    }

    void create_triangle_blas()
    {
      // blas build setup

      // get vertex buffer device address
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address {};
      vertex_buffer_device_address.deviceAddress =
        graphics::get_device_address(device_->get_device(), vertex_buffer_->get_buffer());

      // geometry
      VkAccelerationStructureGeometryKHR as_geometry {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
      };
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      as_geometry.geometry.triangles.vertexData = vertex_buffer_device_address;
      as_geometry.geometry.triangles.maxVertex = triangle_vertices_.size();
      as_geometry.geometry.triangles.vertexStride = sizeof(vec3);
      as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

      // build geometry info
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      // prefer performance rather than as build
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      // get as size
      uint32_t num_triangles = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(
        device_->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &as_build_geometry_info,
        &num_triangles,
        &as_build_sizes_info
      );

      // build blas (get handle of VkAccelerationStructureKHR)
      blas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create blas
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = blas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device_->get_device(), &as_create_info, nullptr, &blas_->handle);

      // get the device address of blas
      VkAccelerationStructureDeviceAddressInfoKHR as_device_address_info {};
      as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      as_device_address_info.accelerationStructure = blas_->handle;
      blas_->device_address = vkGetAccelerationStructureDeviceAddressKHR(device_->get_device(), &as_device_address_info);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // build blas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = blas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute blas build command (in GPU)
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = num_triangles;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;

      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands(graphics::command_type::GRAPHICS);
      vkCmdBuildAccelerationStructuresKHR(
        command, 1, &as_build_geometry_info, as_build_range_infos.data()
      );

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = blas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command, graphics::command_type::GRAPHICS);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    u_ptr<acceleration_structure> create_acceleration_structure_buffer
      (VkAccelerationStructureBuildSizesInfoKHR build_size_info)
    {
      auto as = std::make_unique<acceleration_structure>();
      auto usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        build_size_info.accelerationStructureSize,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        as->buffer,
        as->memory
      );

      return as;
    }

    u_ptr<ray_tracing_scratch_buffer> create_scratch_buffer(VkDeviceSize size)
    {
      auto scratch_buffer = std::make_unique<ray_tracing_scratch_buffer>();
      auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratch_buffer->handle,
        scratch_buffer->memory
      );

      scratch_buffer->device_address = graphics::get_device_address(device_->get_device(), scratch_buffer->handle);
      return scratch_buffer;
    }

    void create_triangle_tlas()
    {
      VkTransformMatrixKHR transform_matrix = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f
      };

      VkAccelerationStructureInstanceKHR as_instance {};
      as_instance.transform = transform_matrix;
      as_instance.instanceCustomIndex = 0;
      as_instance.mask = 0xFF;
      as_instance.instanceShaderBindingTableRecordOffset = 0;
      as_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      as_instance.accelerationStructureReference = blas_->device_address;

      auto device = device_->get_device();
      VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
      VkMemoryPropertyFlags host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      VkDeviceSize buffer_size = sizeof(VkAccelerationStructureInstanceKHR);

      // create instances buffer
      instances_buffer_ = std::make_unique<graphics::buffer>(
        *device_,
        buffer_size,
        1,
        usage,
        host_memory_props
      );

      // write the data to the buffer
      instances_buffer_->map();
      instances_buffer_->write_to_buffer((void *) &as_instance);

      // compute required memory size
      VkDeviceOrHostAddressConstKHR instance_data_device_address {};
      instance_data_device_address.deviceAddress = graphics::get_device_address(device_->get_device(), instances_buffer_->get_buffer());

      VkAccelerationStructureGeometryKHR as_geometry {};
      as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
      as_geometry.geometry.instances.data = instance_data_device_address;

      // get size
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      uint32_t primitive_count = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                              &as_build_geometry_info, &primitive_count, &as_build_sizes_info);

      // create tlas
      tlas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create tlas buffer
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = tlas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device, &as_create_info, nullptr, &tlas_->handle);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // set up for building tlas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = tlas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute build command
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = primitive_count;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;
      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands(graphics::command_type::GRAPHICS);
      vkCmdBuildAccelerationStructuresKHR(command, 1, &as_build_geometry_info, as_build_range_infos.data());

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = tlas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command, graphics::command_type::GRAPHICS);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    void create_ray_traced_image()
    {
      // temporary : for format info
      auto extent = window_->get_extent();
      auto format = back_buffer_format_.format;
      VkImageUsageFlags usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT;
      VkMemoryPropertyFlags device_memory_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      // create image
      ray_traced_image_ = graphics::image_resource::create(
        *device_,
        { extent.width, extent.height, 1 },
        format,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        device_memory_props,
        true); // for ray tracing

      ray_traced_image_->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL);
    }

    void create_layout()
    {
      desc_layout_ = graphics::desc_layout::builder(*device_)
        .add_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .build();

      VkPipelineLayoutCreateInfo pl_create_info {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
      };
      pl_create_info.setLayoutCount = 1;
      pl_create_info.pSetLayouts = desc_layout_->get_p_descriptor_set_layout();
      vkCreatePipelineLayout(device_->get_device(), &pl_create_info, nullptr, &pipeline_layout_);
    }

    void create_pipeline()
    {
      auto ray_generation_stage = load_shader("simple_triangle.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
      auto miss_stage = load_shader("simple_triangle.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
      auto closest_hit_stage = load_shader("simple_triangle.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

      std::vector<VkPipelineShaderStageCreateInfo> stages = {
        ray_generation_stage,
        miss_stage,
        closest_hit_stage,
      };

      const int index_ray_generation = 0;
      const int index_miss = 1;
      const int index_closest_hit = 2;

      // create shader group
      auto ray_generation_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      ray_generation_group.generalShader = index_ray_generation;
      ray_generation_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      ray_generation_group.closestHitShader = VK_SHADER_UNUSED_KHR;
      ray_generation_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      ray_generation_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      auto miss_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      miss_group.generalShader = index_miss;
      miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
      miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      auto closest_hit_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      closest_hit_group.generalShader = VK_SHADER_UNUSED_KHR;
      closest_hit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
      closest_hit_group.closestHitShader = index_closest_hit;
      closest_hit_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      closest_hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      // shader_group_create_infos_.resize(static_cast<size_t>(shader_stages::MAX_STAGE));
      shader_group_create_infos_.resize(3);
      shader_group_create_infos_[static_cast<int>(shader_stages::RAY_GENERATION)] = ray_generation_group;
      shader_group_create_infos_[static_cast<int>(shader_stages::MISS)] = miss_group;
      shader_group_create_infos_[static_cast<int>(shader_stages::CLOSEST_HIT)] = closest_hit_group;

      // create pipeline
      VkRayTracingPipelineCreateInfoKHR pipeline_create_info {
        VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR
      };
      pipeline_create_info.stageCount = uint32_t(stages.size());
      pipeline_create_info.pStages = stages.data();
      pipeline_create_info.groupCount = uint32_t(shader_group_create_infos_.size());
      pipeline_create_info.pGroups = shader_group_create_infos_.data();
      pipeline_create_info.maxPipelineRayRecursionDepth = 1;
      pipeline_create_info.layout = pipeline_layout_;
      vkCreateRayTracingPipelinesKHR(
        device_->get_device(),
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1,
        &pipeline_create_info,
        nullptr,
        &pipeline_
      );

      // delete shader modules
      for (auto& stage : stages) {
        vkDestroyShaderModule(device_->get_device(), stage.module, nullptr);
      }
    }

    VkPipelineShaderStageCreateInfo load_shader(const char* shader_name, VkShaderStageFlagBits stage)
    {
      VkPipelineShaderStageCreateInfo shader_create_info {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr
      };

      auto shader_spv = utils::read_file_for_shader(SHADERS_DIRECTORY + shader_name);
      VkShaderModuleCreateInfo module_create_info {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr
      };
      module_create_info.codeSize = uint32_t(shader_spv.size());
      module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_spv.data());

      VkShaderModule shader_module;
      vkCreateShaderModule(device_->get_device(), &module_create_info, nullptr, &shader_module);

      shader_create_info.stage = stage;
      shader_create_info.pName = "main";
      shader_create_info.module = shader_module;

      return shader_create_info;
    }

    void create_shader_binding_table()
    {
      auto memory_props =
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      auto usage =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT; // additional
      const auto pipeline_props = get_ray_tracing_pipeline_properties();

      // compute size of each entry
      const auto handle_size = pipeline_props.shaderGroupHandleSize;
      const auto handle_alignment = pipeline_props.shaderGroupHandleAlignment;
      auto shader_entry_size = align(handle_size, handle_alignment);

      const auto raygen_shader_count = 1;
      const auto miss_shader_count = 1;
      const auto hit_shader_count = 1;

      const auto base_align = pipeline_props.shaderGroupBaseAlignment;
      auto region_raygen = align(shader_entry_size * raygen_shader_count, base_align);
      auto region_miss   = align(shader_entry_size * miss_shader_count, base_align);
      auto region_hit    = align(shader_entry_size * hit_shader_count, base_align);

      shader_binding_table_ = std::make_unique<graphics::buffer>(
        *device_,
        region_raygen + region_miss + region_hit,
        1,
        usage,
        memory_props,
        0
      );

      // get shader group handle
      auto handle_size_aligned = align(handle_size, handle_alignment);
      auto handle_storage_size = shader_group_create_infos_.size() * handle_size_aligned;
      std::vector<uint8_t> shader_handle_storage(handle_storage_size);
      vkGetRayTracingShaderGroupHandlesKHR(
        device_->get_device(),
        pipeline_,
        0,
        uint32_t(shader_group_create_infos_.size()),
        shader_handle_storage.size(),
        shader_handle_storage.data()
      );

      // write raygen shader entry
      auto device_address = graphics::get_device_address(device_->get_device(), shader_binding_table_->get_buffer());
      shader_binding_table_->map(VK_WHOLE_SIZE, 0);
      auto dst = static_cast<uint8_t*>(shader_binding_table_->get_mapped_memory());

      auto raygen = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::RAY_GENERATION);
      memcpy(dst, raygen, handle_size);
      dst += region_raygen;
      region_raygen_.deviceAddress = device_address;
      region_raygen_.stride = shader_entry_size;
      region_raygen_.size = region_raygen_.stride;

      // write miss shader entry
      auto miss = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::MISS);
      memcpy(dst, miss, handle_size);
      dst += region_miss;
      region_miss_.deviceAddress = device_address + region_raygen;
      region_miss_.size = region_miss;
      region_miss_.stride = shader_entry_size;

      // write hit shader entry
      auto hit = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::CLOSEST_HIT);
      memcpy(dst, hit, handle_size);
      dst += region_hit;
      region_hit_.deviceAddress = device_address + region_raygen + region_miss;
      region_hit_.size = region_hit;
      region_hit_.stride = shader_entry_size;
    }

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR get_ray_tracing_pipeline_properties()
    {
      // get data for size and order of shader entries
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipeline_properties {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
      };
      VkPhysicalDeviceProperties2 device_properties2 {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
      };
      device_properties2.pNext = &pipeline_properties;
      vkGetPhysicalDeviceProperties2(device_->get_physical_device(), &device_properties2);
      return pipeline_properties;
    }

    void create_descriptor_set()
    {
      // create descriptor pool
      desc_pool_ = graphics::desc_pool::builder(*device_)
        .set_max_sets(100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000)
        .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .build();

      // write
      VkWriteDescriptorSetAccelerationStructureKHR as_info {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR
      };
      as_info.accelerationStructureCount = 1;
      as_info.pAccelerationStructures = &tlas_->handle;

      VkDescriptorImageInfo image_info {};
      image_info.imageView = ray_traced_image_->get_image_view();
      image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      graphics::desc_writer(*desc_layout_, *desc_pool_)
        .write_acceleration_structure(0, &as_info)
        .write_image(1, &image_info)
        .build(desc_set_);
    }

    VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout, const void* pNext = nullptr)
    {
      VkDescriptorSetAllocateInfo allocate_info {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr
      };
//      allocate_info.descriptorPool =
      allocate_info.pSetLayouts = &layout;
      allocate_info.descriptorSetCount = 1;
      allocate_info.pNext = pNext;
      VkDescriptorSet descriptor_set;
      if (vkAllocateDescriptorSets(device_->get_device(), &allocate_info, &descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set");
      }
      return descriptor_set;
    }

    void destroy_acceleration_structure(acceleration_structure& as)
    {
      auto device = device_->get_device();
      vkDestroyAccelerationStructureKHR(device, as.handle, nullptr);
      vkFreeMemory(device, as.memory, nullptr);
      vkDestroyBuffer(device, as.buffer, nullptr);
    }

    void destroy_image_resource(graphics::image_resource& ir)
    {
      auto device = device_->get_device();
      vkDestroyImage(device, ir.get_image(), nullptr);
      vkDestroyImageView(device, ir.get_image_view(), nullptr);
      vkFreeMemory(device, ir.get_memory(), nullptr);
    }

    // temporary rendering system
    void wait_available_frame()
    {
      auto timeout = UINT64_MAX;
      auto result = vkAcquireNextImageKHR(
        device_->get_device(),
        swap_chain_,
        timeout,
        present_completed_,
        VK_NULL_HANDLE,
        &frame_index_
      );
      auto fence = command_buffers_[frame_index_]->fence;
      vkWaitForFences(device_->get_device(), 1, &fence, VK_TRUE, timeout);
    }

    void submit_current_frame_command_buffer()
    {
      auto command = command_buffers_[frame_index_]->command;
      VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo submit_info {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1, &present_completed_,
        &wait_stage_mask,
        1, &command,
        1, &render_completed_,
      };
      auto fence = command_buffers_[frame_index_]->fence;
      vkResetFences(device_->get_device(), 1, &fence);
      vkQueueSubmit(device_->get_graphics_queue(), 1, &submit_info, fence);
    }

    void present()
    {
      VkPresentInfoKHR present_info {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1, &render_completed_,
        1, &swap_chain_,
        &frame_index_
      };
      vkQueuePresentKHR(device_->get_present_queue(), &present_info);
    }

    void create_swap_chain()
    {
      surface_extent_ = window_->get_extent();

      VkResult result;
      // create surface
      glfwCreateWindowSurface(device_->get_instance(), window_->get_glfw_window(), nullptr, &surface_);

      VkSurfaceCapabilitiesKHR surface_caps {};
      if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->get_physical_device(), surface_, &surface_caps) != VK_SUCCESS)
        throw std::runtime_error("failed to get surface capabilities.");

      uint32_t count = 0;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device_->get_physical_device(), surface_, &count, nullptr);
      std::vector<VkSurfaceFormatKHR> formats(count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device_->get_physical_device(), surface_, &count, formats.data());
      auto select_format = VkSurfaceFormatKHR{ VK_FORMAT_UNDEFINED };

      auto compare_format = [=, this](auto f) {
        return f.format == back_buffer_format_.format && f.colorSpace == back_buffer_format_.colorSpace;
      };

      if (auto it = std::find_if(formats.begin(), formats.end(), compare_format); it != formats.end()) {
        select_format = *it;
      } else {
        it = std::find_if(formats.begin(), formats.end(), [=, this](auto f) { return f.colorSpace == back_buffer_format_.colorSpace; });
        if (it != formats.end()) {
          select_format = *it;
        } else {
          throw std::runtime_error("failed to get compatible surface format");
        }
      }

      back_buffer_format_ = select_format;

      VkBool32 is_supported;
      if (vkGetPhysicalDeviceSurfaceSupportKHR(
        device_->get_physical_device(),
        device_->get_queue_family_indices().graphics_family_.value(),
        surface_,
        &is_supported) != VK_SUCCESS) {
        throw std::runtime_error("failed to get surface support");
      }

      auto back_buffer_count = surface_caps.minImageCount + 1;

      surface_extent_ = surface_caps.minImageExtent;

      VkSwapchainCreateInfoKHR swap_chain_create_info {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr, 0,
        surface_,
        back_buffer_count,
        back_buffer_format_.format,
        back_buffer_format_.colorSpace,
        surface_extent_,
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        surface_caps.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
        VK_TRUE,
        VK_NULL_HANDLE
      };
      if (vkCreateSwapchainKHR(device_->get_device(), &swap_chain_create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
      }

      uint32_t image_count = 0;
      vkGetSwapchainImagesKHR(device_->get_device(), swap_chain_, &image_count, nullptr);

      std::vector<VkImage> images(image_count);
      vkGetSwapchainImagesKHR(device_->get_device(), swap_chain_, &image_count, images.data());

      render_targets_.resize(image_count);
      for (uint32_t i = 0; i < image_count; ++i) {
        render_targets_[i] = graphics::image_resource::create_blank(*device_);
        render_targets_[i]->set_image(images[i]);

        VkImageViewCreateInfo view_create_info {
          VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          nullptr, 0,
          images[i],
          VK_IMAGE_VIEW_TYPE_2D,
          back_buffer_format_.format,
          VkComponentMapping{
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
          },
          render_targets_[i]->get_sub_resource_range(),
        };

        VkImageView image_view;
        if (vkCreateImageView(device_->get_device(), &view_create_info, nullptr, &image_view) != VK_SUCCESS) {
          throw std::runtime_error("failed to create  image view");
        }
        render_targets_[i]->set_image_view(image_view);
      }

      for (uint32_t i = 0; i < image_count; i++) {
        render_targets_[i]->transition_image_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
      }

      VkSemaphoreCreateInfo semaphore_create_info {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0,
      };
      vkCreateSemaphore(device_->get_device(), &semaphore_create_info, nullptr, &render_completed_);
      vkCreateSemaphore(device_->get_device(), &semaphore_create_info, nullptr, &present_completed_);

      command_buffers_.resize(image_count);
      for (auto& cb : command_buffers_) {
        cb = std::make_unique<frame_command_buffer>();
        cb->command = device_->create_command_buffers(1, graphics::command_type::GRAPHICS)[0];
        cb->fence = create_fence();
      }
    }

    VkFence create_fence()
    {
      VkFenceCreateInfo create_info {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        VK_FENCE_CREATE_SIGNALED_BIT
      };
      VkFence fence;
      vkCreateFence(device_->get_device(), &create_info, nullptr, &fence);
      return fence;
    }

    // ----------------------------------------------------------------------------------------------
    // variables
    u_ptr<graphics::window>   window_;
    u_ptr<graphics::device>   device_;
    u_ptr<graphics::buffer>   vertex_buffer_;
    u_ptr<graphics::buffer>   instances_buffer_;
    u_ptr<graphics::image_resource>              ray_traced_image_;
    std::vector<u_ptr<graphics::image_resource>> back_buffers_;

    std::vector<vec3> triangle_vertices_ = {
      {-0.5f, -0.5f, 0.0f},
      {+0.5f, -0.5f, 0.0f},
      {0.0f,  0.75f, 0.0f}
    };

    // acceleration structure
    u_ptr<acceleration_structure> blas_;
    u_ptr<acceleration_structure> tlas_;

    VkPipelineLayout pipeline_layout_;
    VkPipeline       pipeline_;

    u_ptr<graphics::desc_layout>     desc_layout_;
    s_ptr<graphics::desc_pool>       desc_pool_;
    VkDescriptorSet                  desc_set_;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_group_create_infos_;
    u_ptr<graphics::buffer> shader_binding_table_;

    VkStridedDeviceAddressRegionKHR region_raygen_;
    VkStridedDeviceAddressRegionKHR region_miss_;
    VkStridedDeviceAddressRegionKHR region_hit_;

    // temporal rendering system
    std::vector<u_ptr<graphics::image_resource>> render_targets_;
    VkSemaphore render_completed_;
    VkSemaphore present_completed_;

    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkExtent2D   surface_extent_;
    VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;

    VkSurfaceFormatKHR back_buffer_format_ = {
      VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    };

    uint32_t frame_index_ = 0;
    struct frame_command_buffer {
      VkCommandBuffer command;
      VkFence fence;
    };
    std::vector<u_ptr<frame_command_buffer>> command_buffers_;
};
}

int main() {
  hnll::hello_model app {};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}