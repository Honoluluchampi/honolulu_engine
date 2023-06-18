// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/ray_tracing_system.hpp>
#include <game/actors/default_camera.hpp>
#include <graphics/acceleration_structure.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/utils.hpp>
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>
#include <audio/convolver.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/utils.hpp>

#include "impulse_response.hpp"

// external
#include <AudioFile/AudioFile.h>

// std
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace hnll {

const std::string SHADERS_DIRECTORY =
  std::string(std::getenv("HNLL_ENGN")) + "/examples/ray_tracing/scene_objects/shaders/spv/";

#define MODEL_NAME utils::get_full_path("interior_with_sound.obj")

#define IR_X 50
#define IR_Y 50

#define AUDIO_SEGMENT_NUM 4096

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
    model_ray_tracer(graphics::device& device)
      : game::ray_tracing_system<model_ray_tracer, utils::shading_type::RAY1>(device),
        gui_engine_(utils::singleton<game::gui_engine>::get_instance()){}
    ~model_ray_tracer()
    { audio::engine::kill_hae_context(); }

    void render(const utils::graphics_frame_info& frame_info)
    {
      update_audio(frame_info.frame_index);

      set_current_command(frame_info.command_buffer);

      gui_engine_.transition_vp_image_layout(
        frame_info.frame_index,
        VK_IMAGE_LAYOUT_GENERAL,
        frame_info.command_buffer
      );

      bind_pipeline();
      bind_desc_sets({
        scene_desc_set_,
        vp_desc_sets_[frame_info.frame_index],
        frame_info.global_descriptor_set
      });
      dispatch_command(
        static_cast<uint32_t>(960 * (1.f - 0.2f)),
        static_cast<uint32_t>(820 * (1.f - 0.25f)),
        1
      );

      gui_engine_.transition_vp_image_layout(
        frame_info.frame_index,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        frame_info.command_buffer
      );

      // execute acoustic ray tracing, and copy result to back buffer
      std::vector<VkDescriptorSet> sound_desc_sets {
        scene_desc_set_,
        ir_image_descs_[frame_info.frame_index],
        frame_info.global_descriptor_set
      };
      vkCmdBindPipeline(current_command_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, sound_sbt_->get_pipeline());
      vkCmdBindDescriptorSets(
        current_command_,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        sound_sbt_->get_pipeline_layout(),
        0,
        sound_desc_sets.size(),
        sound_desc_sets.data(),
        0,
        nullptr
      );

      vkCmdTraceRaysKHR(
        current_command_,
        sound_sbt_->get_gen_region_p(),
        sound_sbt_->get_miss_region_p(),
        sound_sbt_->get_hit_region_p(),
        sound_sbt_->get_callable_region_p(),
        IR_X, IR_Y, 1
      );

      // TODO : set barrier
      // copy the result
      auto index = frame_info.frame_index;
      ir_images_[index]->transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, current_command_);
      device_.copy_image_to_buffer(
        ir_images_[index]->get_image(),
        ir_back_buffers_[index]->get_buffer(),
        IR_X,
        IR_Y,
        1,
        current_command_
      );
      ir_images_[index]->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL, current_command_);

    }

    void setup()
    {
      // acceleration structure ----------------------------------------------------------
      // create blas
      mesh_model_ = object::create(device_, MODEL_NAME);

      setup_tlas();

      // desc sets -----------------------------------------------------------------------
      // setup layout
      // tlas and vertex, index buffer layout
      // get desc image layout by gui::renderer
      auto scene_desc_layout = graphics::desc_layout::builder(device_)
        .add_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // tlas
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // vertex buffer
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // index buffer
        .build();
      auto vp_desc_layout = gui_engine_.get_vp_image_desc_layout();
      vp_desc_sets_ = gui_engine_.get_vp_image_desc_sets();

      // setup desc sets
      // also assign ir image desc set
      scene_desc_pool_ = graphics::desc_pool::builder(device_)
        .set_max_sets(10)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10)
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

      // ir image
      ir_images_.resize(utils::FRAMES_IN_FLIGHT);
      ir_back_buffers_.resize(utils::FRAMES_IN_FLIGHT);
      ir_mapped_pointers_.resize(utils::FRAMES_IN_FLIGHT);
      ir_image_descs_.resize(utils::FRAMES_IN_FLIGHT);

      for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
        // actual image
        ir_images_[i] = graphics::image_resource::create(
          device_,
          { IR_X, IR_Y, 1},
          VK_FORMAT_R32G32B32A32_SFLOAT,
          VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        ir_images_[i]->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL);

        // copy acoustic ray tracing result to this buffer
        ir_back_buffers_[i] = graphics::buffer::create(
          device_,
          IR_X * IR_Y * sizeof(vec4),
          1,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          nullptr
        );
        ir_mapped_pointers_[i] = reinterpret_cast<float*>(ir_back_buffers_[i]->get_mapped_memory());
      }

      // ir image desc
      auto ir_desc_layout = graphics::desc_layout::builder(device_)
        .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR) // vertex buffer
        .build();

      for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo image_info {
          .imageView = ir_images_[i]->get_image_view(),
          .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };
        graphics::desc_writer(*ir_desc_layout, *scene_desc_pool_)
          .write_image(0, &image_info)
          .build(ir_image_descs_[i]);
      }

      // shader binding table ------------------------------------------------------
      sbt_ = graphics::shader_binding_table::create(
        device_,
        {
          scene_desc_layout->get_descriptor_set_layout(),
          vp_desc_layout,
          game::graphics_engine_core::get_global_desc_layout(),
        },
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

      sound_sbt_ = graphics::shader_binding_table::create(
        device_,
        {
          scene_desc_layout->get_descriptor_set_layout(),
          vp_desc_layout,
          game::graphics_engine_core::get_global_desc_layout(),
        },
        {
          SHADERS_DIRECTORY + "sound.rgen.spv",
          SHADERS_DIRECTORY + "sound.rmiss.spv",
          SHADERS_DIRECTORY + "sound.rchit.spv",
        },
        {
          VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          VK_SHADER_STAGE_MISS_BIT_KHR,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
        }
      );

      setup_audio();
    }

    void setup_tlas()
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

    void setup_audio()
    {
      audio_file_.load("/home/honolulu/programs/honolulu_engine/sounds/nandesyou.wav");
      audio_file_.printSummary();

      // open al
      hnll::audio::engine::start_hae_context();
      source_ = hnll::audio::engine::get_available_source_id();

      convolver_ = audio::convolver::create(AUDIO_SEGMENT_NUM);
    }

    void update_audio(int frame_index)
    {
      float dt = 0.03; // 16 ms
      int queue_capability = 4;

      static int cursor = 0;
      // continue if queue is full
      hnll::audio::engine::erase_finished_audio_on_queue(source_);

      if (hnll::audio::engine::get_audio_count_on_queue(source_) > queue_capability)
        return;

      // load raw data
      std::vector<ALshort> segment(AUDIO_SEGMENT_NUM, 0.f);
      for (int i = 0; i < AUDIO_SEGMENT_NUM; i++) {
        segment[i] = audio_file_.samples[0][int(i + cursor)] / 2.f;
      }

      auto* p = ir_mapped_pointers_[frame_index];
      std::vector<float> ray_info(p, p + IR_X * IR_Y * 4);

      auto ir_series = unpack_ir(
        ray_info,
        44100,
        AUDIO_SEGMENT_NUM,
        1,
        340
      );

      convolver_->add_segment(std::move(segment), std::move(ir_series));

      cursor += AUDIO_SEGMENT_NUM;

      hnll::audio::audio_data data;
      data.set_sampling_rate(44100)
        .set_format(AL_FORMAT_MONO16)
        .set_data(std::move(convolver_->move_buffer()));

      hnll::audio::engine::bind_audio_to_buffer(data);
      hnll::audio::engine::queue_buffer_to_source(source_, data.get_buffer_id());

      if (!started_) {
        hnll::audio::engine::play_audio_from_source(source_);
        started_ = true;
      }
    }

  private:
    game::gui_engine& gui_engine_;
    u_ptr<object> mesh_model_;
    u_ptr<graphics::acceleration_structure> tlas_;
    u_ptr<graphics::buffer> instances_buffer_;
    std::vector<VkDescriptorSet> vp_desc_sets_;
    VkDescriptorSet scene_desc_set_;
    s_ptr<graphics::desc_pool> scene_desc_pool_;

    // sound
    std::vector<u_ptr<graphics::image_resource>> ir_images_;
    std::vector<VkDescriptorSet>                 ir_image_descs_;
    std::vector<u_ptr<graphics::buffer>>         ir_back_buffers_;
    std::vector<float*>                          ir_mapped_pointers_;
    u_ptr<graphics::shader_binding_table>        sound_sbt_;
    audio::source_id source_;
    AudioFile<short> audio_file_;
    bool started_ = false;

    u_ptr<audio::convolver> convolver_;
};

SELECT_SHADING_SYSTEM(model_ray_tracer);
SELECT_COMPUTE_SHADER();
SELECT_ACTOR(object, game::default_camera);

DEFINE_ENGINE(hello_model) {
  public:
    ENGINE_CTOR(hello_model)
    { add_update_target_directly<game::default_camera>(); }
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