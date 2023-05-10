// hnll
#include <game/shading_systems/static_meshlet_shading_system.hpp>
#include <game/modules/graphics_engine.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/device.hpp>

namespace hnll {

namespace game {

struct meshlet_push_constant
{
  mat4 model_matrix  = mat4::Identity();
  mat4 normal_matrix = mat4::Identity();
};

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(static_meshlet_shading_system, static_meshlet_comp);

void static_meshlet_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESHLET;

  setup_task_desc();

  // prepare desc set layouts
  std::vector<VkDescriptorSetLayout> desc_set_layouts;
  // global
  desc_set_layouts.emplace_back(graphics_engine_core::get_global_desc_layout());
  // task desc
  desc_set_layouts.emplace_back(desc_layout_->get_descriptor_set_layout());
  // meshlet
  auto mesh_descs = graphics::static_meshlet::default_desc_layouts(device_);

  for (auto&& layout : mesh_descs) {
    desc_set_layouts.emplace_back(std::move(layout->get_descriptor_set_layout()));
  }

  pipeline_layout_ = create_pipeline_layout<meshlet_push_constant>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT),
    desc_set_layouts
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_config_info.binding_descriptions.clear();
  pipeline_config_info.attribute_descriptions.clear();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    graphics_engine_core::get_default_render_pass(),
    "/modules/graphics/shader/spv/",
    { "simple_meshlet.task.glsl.spv", "simple_meshlet.mesh.glsl.spv", "simple_meshlet.frag.glsl.spv" },
    { VK_SHADER_STAGE_TASK_BIT_NV, VK_SHADER_STAGE_MESH_BIT_NV, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}


void static_meshlet_shading_system::setup_task_desc()
{
  // pool
  task_desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, utils::FRAMES_IN_FLIGHT)
    .build();

  // desc sets
  const std::vector<graphics::binding_info> bindings = {
    { VK_SHADER_STAGE_TASK_BIT_NV, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
  };

  graphics::desc_set_info set_info { bindings };

  desc_layout_ = graphics::desc_layout::create_from_bindings(device_, bindings);

  // buffer
  for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    // buffer has no actual data at this point
    auto new_buffer = graphics::buffer::create(
      device_,
      sizeof(utils::frustum_info),
      1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      nullptr
    );
    task_desc_sets_->set_buffer(0, 0, i, std::move(new_buffer));
  }

  task_desc_sets_->build();
}

void static_meshlet_shading_system::render(const utils::graphics_frame_info &frame_info)
{
  set_current_command_buffer(frame_info.command_buffer);
  bind_pipeline();

  for (auto &target: targets_) {
    const auto& obj = target.second;

    // update push
    meshlet_push_constant push{};
    push.model_matrix = obj.get_transform().transform_mat4().cast<float>();
    push.normal_matrix = obj.get_transform().normal_matrix().cast<float>();
    bind_push(push, VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT);

    // update task desc set
    size_t index = frame_info.frame_index;
    task_desc_sets_->write_to_buffer(0, 0, index, (void *) &frame_info.view_frustum);
    task_desc_sets_->flush_buffer(0, 0, index);

    const auto& mesh_desc_sets = obj.get_model().get_desc_sets();

    std::vector<VkDescriptorSet> vk_desc_sets = {
      frame_info.global_descriptor_set,
      task_desc_sets_->get_vk_desc_sets(frame_info.frame_index)[0],
      mesh_desc_sets[0],
      mesh_desc_sets[1]
    };

    bind_desc_sets(vk_desc_sets);

    obj.draw(frame_info.command_buffer);
  }
}

}} // namespace hnll::game