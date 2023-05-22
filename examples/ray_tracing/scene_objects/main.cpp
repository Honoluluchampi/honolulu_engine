// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/acceleration_structure.hpp>
#include <utils/rendering_utils.hpp>

using hnll::graphics::image_resource;
using hnll::get_device_address;

namespace hnll {

const std::string SHADERS_DIRECTORY = std::string(std::getenv("HNLL_ENGN")) +
                                      "/applications/ray_tracing/scene_objects/shaders/spv/";

using vec3 = Eigen::Vector3f;
using vec4 = Eigen::Vector4f;
template<typename T> using u_ptr = std::unique_ptr<T>;
template<typename T> using s_ptr = std::shared_ptr<T>;

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

struct vertex
{
  vec3 position;
  vec3 normal;
  vec4 color;
};

struct mesh_model
{
  u_ptr<graphics::buffer> vertex_buffer = nullptr;
  u_ptr<graphics::buffer> index_buffer = nullptr;
  uint32_t vertex_count = 0;
  uint32_t index_count = 0;
  uint32_t vertex_stride = 0;
  uint32_t hit_shader_index = 0;
  u_ptr<graphics::acceleration_structure> blas = nullptr;
};

struct ray_tracing_scratch_buffer
{
  VkBuffer        handle = VK_NULL_HANDLE;
  VkDeviceMemory  memory = VK_NULL_HANDLE;
  VkDeviceAddress device_address = 0;
};

template<class T> T align(T size, uint32_t align)
{ return (size + align - 1) & ~static_cast<T>(align - 1); }

// receives 3x4 matrix
VkTransformMatrixKHR convert_transform(const Eigen::Matrix4f& matrix)
{
  VkTransformMatrixKHR ret {};
  memcpy(&ret.matrix[0], &matrix(0), sizeof(float) * 4);
  memcpy(&ret.matrix[1], &matrix(1), sizeof(float) * 4);
  memcpy(&ret.matrix[2], &matrix(2), sizeof(float) * 4);
  return ret;
}

class hello_triangle {
  public:
    hello_triangle()
    {
      window_ = std::make_unique<graphics::window>(1920, 1080, "hello ray tracing triangle");
      device_ = std::make_unique<graphics::device>(*window_, utils::rendering_type::RAY_TRACING);

      create_scene();
    }

    ~hello_triangle()
    {
      auto device = device_->get_device();
      destroy_image_resource(*ray_traced_image_);
      vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
      vkDestroyPipeline(device, pipeline_, nullptr);
      vkDestroySemaphore(device, render_completed_, nullptr);
      vkDestroySemaphore(device, present_completed_, nullptr);
      for (auto& cb : command_buffers_) {
        vkDestroyFence(device, cb->fence, nullptr);
      }
      for (auto& rt : render_targets_) {
        vkDestroyImageView(device, rt->get_image_view(), nullptr);
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
        &descriptor_set_,
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

      ray_traced_image_->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
      back_buffer->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      vkCmdCopyImage(
        command,
        ray_traced_image_->get_image(),
        ray_traced_image_->get_image_layout(),
        back_buffer->get_image(),
        back_buffer->get_image_layout(),
        1,
        &region
      );

      ray_traced_image_->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_GENERAL);
      back_buffer->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

      vkEndCommandBuffer(command);

      submit_current_frame_command_buffer();
      present();
    }

    void create_scene()
    {
      create_swap_chain();
      create_scene_geometries();
      create_scene_blas();
      create_scene_tlas();
      create_ray_traced_image();
      create_layout();
      create_pipeline();
      create_shader_binding_table();
      create_descriptor_set();
    }

    void create_ray_traced_image()
    {
      // temporary : for format info
      auto extent = window_->get_extent();
      auto format = back_buffer_format_.format;
      auto device = device_->get_device();
      VkImageUsageFlags usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT;
      VkMemoryPropertyFlags device_memory_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      // create image
      ray_traced_image_ = create_texture_2d(extent, format, usage, device_memory_props);

      auto command = device_->begin_one_shot_commands();
      ray_traced_image_->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_GENERAL);
      device_->end_one_shot_commands(command);
    }

    u_ptr<image_resource> create_texture_2d(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_props)
    {
      auto res = std::make_unique<image_resource>();

      // create image
      VkImageCreateInfo create_info {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr
      };
      create_info.imageType = VK_IMAGE_TYPE_2D;
      create_info.format = format;
      create_info.extent = {extent.width, extent.height, 1};
      create_info.mipLevels = 1;
      create_info.arrayLayers = 1;
      create_info.samples = VK_SAMPLE_COUNT_1_BIT;
      create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
      create_info.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      VkImage image;
      VkDeviceMemory image_memory;

      device_->create_image_with_info(create_info, memory_props, image, image_memory);

      // create image view
      VkImageViewCreateInfo view_create_info {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr
      };
      view_create_info.image = image;
      view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_create_info.format = format;
      view_create_info.components = VkComponentMapping{
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
      };
      view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

      VkImageView view;
      vkCreateImageView(device_->get_device(), &view_create_info, nullptr, &view);

      res->set_image(image);
      res->set_image_view(view);
      res->set_device_memory(image_memory);

      return res;
    }

    void create_layout()
    {
      descriptor_set_layout_ = graphics::descriptor_set_layout::builder(*device_)
        .add_binding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
          // make it dynamic because the ubo is writen by cpu accessed by gpu
        .add_binding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_ALL)
        .build();

      VkPipelineLayoutCreateInfo pl_create_info {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
      };
      pl_create_info.setLayoutCount = 1;
      pl_create_info.pSetLayouts = descriptor_set_layout_->get_p_descriptor_set_layout();
      vkCreatePipelineLayout(device_->get_device(), &pl_create_info, nullptr, &pipeline_layout_);
    }

    void create_pipeline()
    {
      auto ray_generation_stage = load_shader("ray_generation.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
      auto miss_stage = load_shader("miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
      auto closest_hit_stage = load_shader("closest_hit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

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

      auto shader_spv = graphics::pipeline::read_file(SHADERS_DIRECTORY + shader_name);
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
      auto device_address = get_device_address(device_->get_device(), shader_binding_table_->get_buffer());
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
      descriptor_pool_ = graphics::descriptor_pool::builder(*device_)
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
      auto handle = tlas_->get_as_handle();
      as_info.pAccelerationStructures = &handle;

      VkDescriptorImageInfo image_info {};
      image_info.imageView = ray_traced_image_->get_image_view();
      image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      graphics::descriptor_writer(*descriptor_set_layout_, *descriptor_pool_)
        .write_acceleration_structure(0, &as_info)
        .write_image(1, &image_info)
        .build(descriptor_set_);
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

    void destroy_image_resource(image_resource& ir)
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
        render_targets_[i] = std::make_unique<image_resource>();
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

      auto command = device_->begin_one_shot_commands();
      for (uint32_t i = 0; i < image_count; i++) {
        render_targets_[i]->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
      }
      device_->end_one_shot_commands(command);

      VkSemaphoreCreateInfo semaphore_create_info {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0,
      };
      vkCreateSemaphore(device_->get_device(), &semaphore_create_info, nullptr, &render_completed_);
      vkCreateSemaphore(device_->get_device(), &semaphore_create_info, nullptr, &present_completed_);

      command_buffers_.resize(image_count);
      for (auto& cb : command_buffers_) {
        cb = std::make_unique<frame_command_buffer>();
        cb->command = create_command_buffer();
        cb->fence = create_fence();
      }
    }

    VkCommandBuffer create_command_buffer()
    {
      VkCommandBufferAllocateInfo allocate_info {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        device_->get_command_pool(),
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
      };
      VkCommandBuffer command;
      vkAllocateCommandBuffers(device_->get_device(), &allocate_info, &command);
      return command;
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

    void create_scene_geometries()
    {
      auto host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      auto usage_for_rt =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
      std::vector<vertex>   vertices;
      std::vector<uint32_t> indices;
      const auto vertex_stride = static_cast<uint32_t>(sizeof(vertex));
      const auto index_stride  = static_cast<uint32_t>(sizeof(uint32_t));

      // create plane mesh_model
      plane_ = std::make_unique<mesh_model>();
      create_plane(vertices, indices);

      auto vertex_buffer_size = vertex_stride * vertices.size();
      auto index_buffer_size  = index_stride * indices.size();

      // create plane vertex buffer
      auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | usage_for_rt;
      plane_->vertex_buffer = std::make_unique<graphics::buffer>(
        *device_,
        vertex_buffer_size,
        1,
        usage,
        host_memory_props
      );
      plane_->vertex_buffer->map();
      plane_->vertex_buffer->write_to_buffer((void *) vertices.data());
      plane_->vertex_count = static_cast<uint32_t>(vertices.size());
      plane_->vertex_stride = vertex_stride;

      // create plane index buffer
      usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | usage_for_rt;
      plane_->index_buffer = std::make_unique<graphics::buffer>(
        *device_,
        index_buffer_size,
        1,
        usage,
        host_memory_props
      );
      plane_->index_buffer->map();
      plane_->index_buffer->write_to_buffer((void *) indices.data());
      plane_->index_count = static_cast<uint32_t>(indices.size());

      // create cube mesh_model
      cube_ = std::make_unique<mesh_model>();
      create_cube(vertices, indices);

      auto cube_vertex_buffer_size = vertex_stride * vertices.size();
      auto cube_index_buffer_size  = index_stride * indices.size();

      // create cube vertex buffer
      usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | usage_for_rt;
      cube_->vertex_buffer = std::make_unique<graphics::buffer>(
        *device_,
        cube_vertex_buffer_size,
        1,
        usage,
        host_memory_props
      );
      cube_->vertex_buffer->map();
      cube_->vertex_buffer->write_to_buffer((void *) vertices.data());
      cube_->vertex_count = static_cast<uint32_t>(vertices.size());
      cube_->vertex_stride = vertex_stride;

      // create cube index buffer
      usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | usage_for_rt;
      cube_->index_buffer = std::make_unique<graphics::buffer>(
        *device_,
        cube_index_buffer_size,
        1,
        usage,
        host_memory_props
      );
      cube_->index_buffer->map();
      cube_->index_buffer->write_to_buffer((void *) indices.data());
      cube_->index_count = static_cast<uint32_t>(indices.size());
    }

    void create_plane(std::vector<vertex>& vertices, std::vector<uint32_t>& indices, float size = 10.f)
    {
      vertices.clear();
      indices.clear();

      const vec4 white = vec4{ 1, 1, 1, 1 };
      vertex src[] = {
        vertex{ { -1.f, 0.f, -1.f }, { 0.f, 1.f, 0.f }, white },
        vertex{ { -1.f, 0.f,  1.f }, { 0.f, 1.f, 0.f }, white },
        vertex{ {  1.f, 0.f, -1.f }, { 0.f, 1.f, 0.f }, white },
        vertex{ {  1.f, 0.f,  1.f }, { 0.f, 1.f, 0.f }, white },
      };
      vertices.resize(4);

      // set size
      std::transform(
        std::begin(src), std::end(src), vertices.begin(),
        [=](auto v) {
          v.position.x() *= size;
          v.position.z() *= size;
          return v;
        }
      );
      indices = { 0, 1, 2, 2, 1, 3};
    }

    void create_cube(std::vector<vertex>& vertices, std::vector<uint32_t>& indices, float size = 1.f)
    {
      vertices.clear();
      indices.clear();

      const vec4 red     { 1.f, 0.f, 0.f, 1.f };
      const vec4 green   { 0.f, 1.f, 0.f, 1.f };
      const vec4 blue    { 0.f, 0.f, 1.f, 1.f };
      const vec4 white   { 1.f, 1.f, 1.f, 1.f };
      const vec4 black   { 0.f, 0.f, 0.f, 1.f };
      const vec4 yellow  { 1.f, 1.f, 0.f, 1.f };
      const vec4 magenta { 1.f, 0.f, 1.f, 1.f };
      const vec4 cyan    { 0.f, 1.f, 1.f, 1.f };

      vertices = {
        { { -1.f, -1.f, -1.f }, {  0.f,  0.f, -1.f }, red },
        { { -1.f,  1.f, -1.f }, {  0.f,  0.f, -1.f }, yellow },
        { {  1.f,  1.f, -1.f }, {  0.f,  0.f, -1.f }, white },
        { {  1.f, -1.f, -1.f }, {  0.f,  0.f, -1.f }, magenta },

        { {  1.f, -1.f, -1.f }, {  1.f,  0.f,  0.f }, magenta },
        { {  1.f,  1.f, -1.f }, {  1.f,  0.f,  0.f }, white },
        { {  1.f,  1.f,  1.f }, {  1.f,  0.f,  0.f }, cyan },
        { {  1.f, -1.f,  1.f }, {  1.f,  0.f,  0.f }, blue },

        { { -1.f, -1.f,  1.f }, { -1.f,  0.f,  0.f }, black },
        { { -1.f,  1.f,  1.f }, { -1.f,  0.f,  0.f }, green },
        { { -1.f,  1.f, -1.f }, { -1.f,  0.f,  0.f }, yellow },
        { { -1.f, -1.f, -1.f }, { -1.f,  0.f,  0.f }, red },

        { {  1.f, -1.f,  1.f }, {  0.f,  0.f,  1.f }, blue },
        { {  1.f,  1.f,  1.f }, {  0.f,  0.f,  1.f }, cyan },
        { { -1.f,  1.f,  1.f }, {  0.f,  0.f,  1.f }, green },
        { { -1.f, -1.f,  1.f }, {  0.f,  0.f,  1.f }, black },

        { { -1.f,  1.f, -1.f }, {  0.f,  1.f,  0.f }, yellow },
        { { -1.f,  1.f,  1.f }, {  0.f,  1.f,  0.f }, green },
        { {  1.f,  1.f,  1.f }, {  0.f,  1.f,  0.f }, cyan },
        { {  1.f,  1.f, -1.f }, {  0.f,  1.f,  0.f }, white },

        { { -1.f, -1.f,  1.f }, {  0.f, -1.f,  0.f }, black },
        { { -1.f, -1.f, -1.f }, {  0.f, -1.f,  0.f }, red },
        { {  1.f, -1.f, -1.f }, {  0.f, -1.f,  0.f }, magenta },
        { {  1.f, -1.f,  1.f }, {  0.f, -1.f,  0.f }, blue },
      };

      indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
      };

      std::transform(
        vertices.begin(), vertices.end(), vertices.begin(),
        [=](auto v) {
          v.position.x() *= size;
          v.position.y() *= size;
          v.position.z() *= size;
          return v;
        }
      );
    }

    void create_scene_blas() {
      auto device = device_->get_device();

      // create plane
      {
        // create geometry
        VkAccelerationStructureGeometryKHR geometry{
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
        };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        auto &triangles = geometry.geometry.triangles;
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = hnll::get_device_address(device, plane_->vertex_buffer->get_buffer());
        triangles.maxVertex = plane_->vertex_count;
        triangles.vertexStride = plane_->vertex_stride;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = hnll::get_device_address(device, plane_->index_buffer->get_buffer());

        // create build range info
        VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
        build_range_info.primitiveCount = plane_->index_count / 3;
        build_range_info.primitiveOffset = 0;
        build_range_info.firstVertex = 0;
        build_range_info.transformOffset = 0;

        // create input
        graphics::acceleration_structure::input blas_input;
        blas_input.geometry = {geometry};
        blas_input.build_range_info = {build_range_info};

        // execute build
        plane_->blas = std::make_unique<graphics::acceleration_structure>(*device_);
        plane_->blas->build_as(
          VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
          blas_input,
          0
        );
        plane_->blas->destroy_scratch_buffer();
        // configure hit shader index
        plane_->hit_shader_index = scene_hit_shader_group::plane_hit_shader;
      }

      // build cube
      {
        VkAccelerationStructureGeometryKHR geometry {
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
        };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        auto& triangles = geometry.geometry.triangles;
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = hnll::get_device_address(device, cube_->vertex_buffer->get_buffer());
        triangles.maxVertex = cube_->vertex_count;
        triangles.vertexStride = cube_->vertex_stride;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = hnll::get_device_address(device, cube_->index_buffer->get_buffer());

        VkAccelerationStructureBuildRangeInfoKHR build_range_info {};
        build_range_info.primitiveCount = cube_->index_count / 3;
        build_range_info.primitiveOffset = 0;
        build_range_info.firstVertex = 0;
        build_range_info.transformOffset = 0;

        graphics::acceleration_structure::input blas_input;
        blas_input.geometry = { geometry };
        blas_input.build_range_info = { build_range_info };

        cube_->blas = std::make_unique<graphics::acceleration_structure>(*device_);

        cube_->blas->build_as(
          VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
          blas_input,
          0
        );
        cube_->blas->destroy_scratch_buffer();
        cube_->hit_shader_index = scene_hit_shader_group::cube_hit_shader;
      }
    }

    void create_scene_tlas()
    {
      auto device = device_->get_device();

      std::vector<VkAccelerationStructureInstanceKHR> instances;
      deploy_objects(instances);

      auto instance_buffer_size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
      auto usage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
      auto host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

      // create and write buffer
      instances_buffer_ = std::make_unique<graphics::buffer>(
        *device_,
        instance_buffer_size,
        1,
        usage,
        host_memory_props
      );
      instances_buffer_->map();
      instances_buffer_->write_to_buffer((void *) instances.data());

      // get device address
      VkDeviceOrHostAddressConstKHR instance_device_address {};
      instance_device_address.deviceAddress = hnll::get_device_address(device, instances_buffer_->get_buffer());

      // prepare input for tlas build
      VkAccelerationStructureGeometryKHR geometry {};
      geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      geometry.geometry.instances.arrayOfPointers = VK_FALSE;
      geometry.geometry.instances.data = instance_device_address;

      VkAccelerationStructureBuildRangeInfoKHR build_range_info {};
      build_range_info.primitiveCount = static_cast<uint32_t>(instances.size());
      build_range_info.primitiveOffset = 0;
      build_range_info.firstVertex = 0;
      build_range_info.transformOffset = 0;

      // build
      graphics::acceleration_structure::input tlas_input {};
      tlas_input.geometry = { geometry };
      tlas_input.build_range_info = { build_range_info };

      tlas_ = std::make_unique<graphics::acceleration_structure>(*device_);

      tlas_->build_as(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, tlas_input, 0);
      tlas_->destroy_scratch_buffer();
    }

    void deploy_objects(std::vector<VkAccelerationStructureInstanceKHR>& instances)
    {
      VkAccelerationStructureInstanceKHR template_description {};
      template_description.instanceCustomIndex = 0;
      template_description.mask = 0xFF;
      template_description.flags = 0;

      // plane
      {
        VkTransformMatrixKHR transform = convert_transform(Eigen::Matrix4f::Identity());
        VkAccelerationStructureInstanceKHR instance = template_description;
        instance.transform = transform;
        instance.accelerationStructureReference = plane_->blas->get_device_address();
        instance.instanceShaderBindingTableRecordOffset = plane_->hit_shader_index;
        instances.push_back(instance);
      }
      // cube 1
      {
        Eigen::Matrix4f tf = Eigen::Matrix4f::Identity();
        tf(0, 3) = -2.f;
        tf(1, 3) = 1.f;
        VkTransformMatrixKHR transform = convert_transform(tf);
        VkAccelerationStructureInstanceKHR instance = template_description;
        instance.transform = transform;
        instance.accelerationStructureReference = cube_->blas->get_device_address();
        instance.instanceShaderBindingTableRecordOffset = cube_->hit_shader_index;
        instances.push_back(instance);
      }
      // cube 2
      {
        Eigen::Matrix4f tf = Eigen::Matrix4f::Identity();
        tf(0, 3) = 2.f;
        tf(1, 3) = 1.f;
        VkTransformMatrixKHR transform = convert_transform(tf);
        VkAccelerationStructureInstanceKHR instance = template_description;
        instance.transform = transform;
        instance.accelerationStructureReference = cube_->blas->get_device_address();
        instance.instanceShaderBindingTableRecordOffset = cube_->hit_shader_index;
        instances.push_back(instance);
      }
    }

    // ----------------------------------------------------------------------------------------------
    // variables
    u_ptr<graphics::window>   window_;
    u_ptr<graphics::device>   device_;
    u_ptr<graphics::buffer>   vertex_buffer_;
    u_ptr<graphics::buffer>   instances_buffer_;
    u_ptr<image_resource>     ray_traced_image_;
    std::vector<u_ptr<image_resource>> back_buffers_;

    // each mesh_model has own blas
    u_ptr<mesh_model> plane_;
    u_ptr<mesh_model> cube_;

    // acceleration structure
    u_ptr<graphics::acceleration_structure> tlas_;

    VkPipelineLayout pipeline_layout_;
    VkPipeline       pipeline_;

    u_ptr<graphics::descriptor_set_layout> descriptor_set_layout_;
    u_ptr<graphics::descriptor_pool>       descriptor_pool_;
    VkDescriptorSet                        descriptor_set_;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_group_create_infos_;
    u_ptr<graphics::buffer> shader_binding_table_;

    VkStridedDeviceAddressRegionKHR region_raygen_;
    VkStridedDeviceAddressRegionKHR region_miss_;
    VkStridedDeviceAddressRegionKHR region_hit_;

    // temporary rendering system
    std::vector<u_ptr<image_resource>> render_targets_;
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
  hnll::hello_triangle app {};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// empty
#include <geometry/mesh_model.hpp>
void hnll::geometry::mesh_model::align_vertex_id() {}
#include <utils/utils.hpp>
std::string hnll::utils::get_full_path(const std::string &_filename) {}