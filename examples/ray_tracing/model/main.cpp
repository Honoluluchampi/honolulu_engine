// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/acceleration_structure.hpp>
#include <graphics/shader_binding_table.hpp>
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

template<class T> T align(T size, uint32_t align)
{ return (size + align - 1) & ~static_cast<T>(align - 1); }

class hello_model {
  public:
    hello_model()
    {
      window_ = std::make_unique<graphics::window>(960, 820, "hello ray tracing model");
      device_ = std::make_unique<graphics::device>(
        *window_,
        utils::rendering_type::RAY_TRACING
      );

      create_triangle_as();
    }

    void cleanup()
    {
      auto device = device_->get_device();
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
      // temporal
      // this should be done by game::graphics_engine_core
      graphics::texture_image::reset_desc_layout();
    }

    void run()
    {
      while (glfwWindowShouldClose(window_->get_glfw_window()) == GLFW_FALSE) {
        {
          utils::scope_timer timer{"fps", utils::timer_type::MILLI};
          glfwPollEvents();
          update();
          render();
        }
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
        sbt_->get_pipeline()
      );
      vkCmdBindDescriptorSets(
        command,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        sbt_->get_pipeline_layout(),
        0,
        1,
        &desc_set_,
        0,
        nullptr
      );

      vkCmdTraceRaysKHR(
        command,
        sbt_->get_gen_region_p(),
        sbt_->get_miss_region_p(),
        sbt_->get_hit_region_p(),
        sbt_->get_callable_region_p(),
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
      load_model();
      create_triangle_blas();
      create_triangle_tlas();
      create_ray_traced_image();
      create_layout();
      create_descriptor_set();
    }

    void load_model()
    {
      mesh_model_ = graphics::static_mesh::create_from_file(*device_, MODEL_NAME, true);
      vertex_count_ = mesh_model_->get_vertex_count();
      index_count_  = mesh_model_->get_face_count() * 3;
    }

    void create_triangle_blas()
    {
      // blas build setup
      uint32_t num_triangles = index_count_ / 3;

      // get vertex buffer device address
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address {
        .deviceAddress = graphics::get_device_address(device_->get_device(), mesh_model_->get_vertex_vk_buffer())
      };
      VkDeviceOrHostAddressConstKHR index_buffer_device_address {
        .deviceAddress = graphics::get_device_address(device_->get_device(), mesh_model_->get_index_vk_buffer())
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

      blas_ = graphics::acceleration_structure::create(*device_);
      blas_->build_as(
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        blas_input,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
      );
      blas_->destroy_scratch_buffer();
    }

    void create_triangle_tlas()
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
      as_instance.accelerationStructureReference = blas_->get_as_device_address();

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
      VkDeviceOrHostAddressConstKHR instance_data_device_address {
        .deviceAddress = graphics::get_device_address(device_->get_device(), instances_buffer_->get_buffer())
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

      tlas_ = graphics::acceleration_structure::create(*device_);
      tlas_->build_as(
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        tlas_input,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
      );
      tlas_->destroy_scratch_buffer();
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
        device_memory_props); // for ray tracing

      ray_traced_image_->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL);
    }

    void create_layout()
    {
      desc_layout_ = graphics::desc_layout::builder(*device_)
        .add_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // tlas
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR) // ray traced image
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // vertex buffer
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // index buffer
        .build();

      sbt_ = graphics::shader_binding_table::create(
        *device_,
        { desc_layout_->get_descriptor_set_layout() },
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

    void create_descriptor_set()
    {
      // create descriptor pool
      desc_pool_ = graphics::desc_pool::builder(*device_)
        .set_max_sets(100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000)
        .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .build();

      VkDescriptorImageInfo image_info {};
      image_info.imageView = ray_traced_image_->get_image_view();
      image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      VkWriteDescriptorSetAccelerationStructureKHR as_info = tlas_->get_as_info();

      auto vertex_buffer_info = mesh_model_->get_vertex_buffer_info();
      auto index_buffer_info  = mesh_model_->get_index_buffer_info();

      graphics::desc_writer(*desc_layout_, *desc_pool_)
        .write_acceleration_structure(0, &as_info)
        .write_image(1, &image_info)
        .write_buffer(2, &vertex_buffer_info)
        .write_buffer(3, &index_buffer_info)
        .build(desc_set_);
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
    u_ptr<graphics::static_mesh> mesh_model_;
    u_ptr<graphics::buffer>   instances_buffer_;
    u_ptr<graphics::image_resource>              ray_traced_image_;
    std::vector<u_ptr<graphics::image_resource>> back_buffers_;

    size_t vertex_count_ = 0;
    size_t index_count_ = 0;

    // acceleration structure
    u_ptr<graphics::acceleration_structure> blas_;
    u_ptr<graphics::acceleration_structure> tlas_;

    u_ptr<graphics::shader_binding_table> sbt_;

    u_ptr<graphics::desc_layout>     desc_layout_;
    s_ptr<graphics::desc_pool>       desc_pool_;
    VkDescriptorSet                  desc_set_;

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