#pragma once

#include <game/renderable_component.hpp>
#include <game/shading_template_config.hpp>
#include <graphics/device.hpp>
#include <graphics/pipeline.hpp>

namespace hnll {

namespace game {

template<RenderableComponent RC>
class shading_system {
    using target_map = std::unordered_map<rc_id, RC&>;

  public:
    explicit shading_system(graphics::device &device, utils::shading_type type)
      : device_(device), shading_type_(type) {}

    ~shading_system() { vkDestroyPipelineLayout(device_.get_device(), pipeline_layout_, nullptr); }

    static u_ptr<shading_system<RC>> create(graphics::device& device);

    shading_system(const shading_system &) = delete;
    shading_system &operator=(const shading_system &) = delete;
    shading_system(shading_system &&) = default;
    shading_system &operator=(shading_system &&) = default;

    void render(const utils::frame_info &frame_info) {}

    void add_render_target(rc_id id, RC& target) { targets_.emplace(id, target); }
    void remove_render_target(rc_id id) { targets_.erase(id); }

    // getter
    utils::shading_type get_shading_type() const { return shading_type_; }

    // setter
    shading_system &set_shading_type(utils::shading_type type) { shading_type_ = type; return *this; }

  protected:
    // takes push_constant struct as type parameter (use it for size calculation)
    template<typename PushConstant>
    VkPipelineLayout create_pipeline_layout(
      VkShaderStageFlagBits shader_stage_flags,
      const std::vector<VkDescriptorSetLayout>& desc_set_layouts
    );

    VkPipelineLayout create_pipeline_layout_without_push(
      VkShaderStageFlagBits shader_stage_flags,
      const std::vector<VkDescriptorSetLayout>& desc_set_layouts
    );

    u_ptr<graphics::pipeline> create_pipeline(
      VkPipelineLayout pipeline_layout,
      VkRenderPass render_pass,
      const std::string& shaders_directory,
      const std::vector<std::string>& shader_filenames,
      const std::vector<VkShaderStageFlagBits>& shader_stage_flags,
      graphics::pipeline_config_info config_info
    );

    // vulkan objects
    graphics::device &device_;
    u_ptr<graphics::pipeline> pipeline_;
    VkPipelineLayout pipeline_layout_;

    // shading system is called in rendering_type-order at rendering process
    utils::shading_type shading_type_;
    target_map targets_;
};

// impl
// takes push_constant struct as type parameter
template <RenderableComponent R> template<typename PushConstant>
VkPipelineLayout shading_system<R>::create_pipeline_layout(
  VkShaderStageFlagBits shader_stage_flags,
  const std::vector<VkDescriptorSetLayout>& desc_set_layouts) {
  // configure push constant
  VkPushConstantRange push_constant_range{};
  push_constant_range.stageFlags = shader_stage_flags;
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof(PushConstant);

  // configure desc sets layout
  VkPipelineLayoutCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.setLayoutCount = static_cast<uint32_t>(desc_set_layouts.size());
  create_info.pSetLayouts = desc_set_layouts.data();
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges = &push_constant_range;

  // create
  VkPipelineLayout ret;
  if (vkCreatePipelineLayout(device_.get_device(), &create_info, nullptr, &ret) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout.");

  return ret;
}

template<RenderableComponent RC>
VkPipelineLayout shading_system<RC>::create_pipeline_layout_without_push(
  VkShaderStageFlagBits shader_stage_flags,
  const std::vector<VkDescriptorSetLayout>& desc_set_layouts)
{
  // configure desc sets layout
  VkPipelineLayoutCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.setLayoutCount = static_cast<uint32_t>(desc_set_layouts.size());
  create_info.pSetLayouts = desc_set_layouts.data();
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = nullptr;

  // create
  VkPipelineLayout ret;
  if (vkCreatePipelineLayout(device_.get_device(), &create_info, nullptr, &ret) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout.");

  return ret;
}

// shaders_directory is relative to $ENV{HNLL_ENGN}
template<RenderableComponent RC>
u_ptr<graphics::pipeline> shading_system<RC>::create_pipeline(
  VkPipelineLayout                   pipeline_layout,
  VkRenderPass                       render_pass,
  const std::string&                        shaders_directory,
  const std::vector<std::string>&           shader_filenames,
  const std::vector<VkShaderStageFlagBits>& shader_stage_flags,
  graphics::pipeline_config_info     config_info)
{
  auto directory = std::string(std::getenv("HNLL_ENGN")) + shaders_directory;

  std::vector<std::string> shader_paths;
  for (const auto& name : shader_filenames) {
    shader_paths.emplace_back(directory + name);
  }

  config_info.pipeline_layout = pipeline_layout;
  config_info.render_pass     = render_pass;

  return graphics::pipeline::create(
    device_,
    shader_paths,
    shader_stage_flags,
    config_info
  );
}

template<utils::shading_type T>
class dummy_renderable_comp
{
  public:
    void bind() {}
    void draw() {}
    rc_id get_rc_id() const { return 0; }
    utils::shading_type get_shading_type() const { return type; }
  private:
    utils::shading_type type = T;
};

}} // namespace hnll::game