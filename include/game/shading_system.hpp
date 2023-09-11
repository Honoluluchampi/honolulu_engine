#pragma once

// hnll
#include <game/concepts.hpp>
#include <graphics/device.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/singleton.hpp>

// std
#include <unordered_map>

#define DEFINE_SHADING_SYSTEM(new_system, rc) class new_system : public game::shading_system<new_system, rc>
#define DEFAULT_SHADING_SYSTEM_CTOR(system, rc) explicit system() : game::shading_system<system, rc>() {}
#define DEFAULT_SHADING_SYSTEM_CTOR_DECL(system, rc) explicit system()
#define DEFAULT_SHADING_SYSTEM_CTOR_IMPL(system, rc) system::system() : game::shading_system<system, rc>() {}

namespace hnll {

namespace game {

template<typename Derived, RenderableComponent RC>
class shading_system {
    using target_map = std::unordered_map<rc_id, RC&>;

  public:
    explicit shading_system() : device_(utils::singleton<graphics::device>::get_single_ptr()) {}
    ~shading_system() { vkDestroyPipelineLayout(device_->get_device(), pipeline_layout_, nullptr); }

    static u_ptr<Derived> create() { return std::make_unique<Derived>(); }

    shading_system(const shading_system &) = delete;
    shading_system &operator=(const shading_system &) = delete;
    shading_system(shading_system &&) = default;
    shading_system &operator=(shading_system &&) = default;

    // impl in each shader
    void setup();
    void render(const utils::graphics_frame_info &frame_info);

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

    u_ptr<graphics::pipeline> create_pipeline(
      VkPipelineLayout pipeline_layout,
      VkRenderPass render_pass,
      const std::vector<std::string>& shaders_directories,
      const std::vector<std::string>& shader_filenames,
      const std::vector<VkShaderStageFlagBits>& shader_stage_flags,
      graphics::pipeline_config_info config_info
    );

    // call this before binding
    void set_current_command_buffer(VkCommandBuffer command);

    void bind_pipeline();

    void bind_desc_sets(const std::vector<VkDescriptorSet>& desc_sets);

    template <typename PushConstants>
    void bind_push(const PushConstants& push, VkShaderStageFlags stages);

    // vulkan objects
    utils::single_ptr<graphics::device> device_;
    u_ptr<graphics::pipeline> pipeline_;
    VkPipelineLayout pipeline_layout_;
    u_ptr<graphics::desc_layout> desc_layout_;

    // shading system is called in rendering_type-order at rendering process
    utils::shading_type shading_type_;
    target_map targets_;

    // update this at the start of the command recording
    VkCommandBuffer current_command_;
};

// impl
#define SS_API template <typename Derived, RenderableComponent R>
#define SS_TYPE shading_system<Derived, R>

// takes push_constant struct as type parameter
SS_API template<typename PushConstant> VkPipelineLayout SS_TYPE::create_pipeline_layout(
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
  if (vkCreatePipelineLayout(device_->get_device(), &create_info, nullptr, &ret) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout.");

  return ret;
}

SS_API VkPipelineLayout SS_TYPE::create_pipeline_layout_without_push(
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
  if (vkCreatePipelineLayout(device_->get_device(), &create_info, nullptr, &ret) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout.");

  return ret;
}

// shaders_directory is relative to $ENV{HNLL_ENGN}
SS_API u_ptr<graphics::pipeline> SS_TYPE::create_pipeline(
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
    *device_,
    shader_paths,
    shader_stage_flags,
    config_info
  );
}

SS_API u_ptr<graphics::pipeline> SS_TYPE::create_pipeline(
  VkPipelineLayout                   pipeline_layout,
  VkRenderPass                       render_pass,
  const std::vector<std::string>&           shaders_directories,
  const std::vector<std::string>&           shader_filenames,
  const std::vector<VkShaderStageFlagBits>& shader_stage_flags,
  graphics::pipeline_config_info     config_info)
{
  assert(shaders_directories.size() == shader_filenames.size() && "shader directories and filenames do not correspond to each other");

  std::vector<std::string> shader_paths;
  for (int i = 0; i < shaders_directories.size(); i++) {
    shader_paths.emplace_back(std::string(std::getenv("HNLL_ENGN")) + shaders_directories[i] + shader_filenames[i]);
  }

  config_info.pipeline_layout = pipeline_layout;
  config_info.render_pass     = render_pass;

  return graphics::pipeline::create(
    *device_,
    shader_paths,
    shader_stage_flags,
    config_info
  );
}

SS_API void SS_TYPE::set_current_command_buffer(VkCommandBuffer command)
{ current_command_ = command; }

SS_API void SS_TYPE::bind_pipeline()
{ pipeline_->bind(current_command_); }

SS_API void SS_TYPE::bind_desc_sets(const std::vector<VkDescriptorSet> &desc_sets)
{
  vkCmdBindDescriptorSets(
    current_command_,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline_layout_,
    0,
    desc_sets.size(),
    desc_sets.data(),
    0,
    nullptr
  );
}

SS_API template <typename PushConstants> void SS_TYPE::bind_push(
  const PushConstants &push,
  VkShaderStageFlags stages)
{
  vkCmdPushConstants(
    current_command_,
    pipeline_layout_,
    stages,
    0,
    sizeof(PushConstants),
    &push
  );
}

template<utils::shading_type T>
class dummy_renderable_comp
{
  public:
    void bind(VkCommandBuffer cb) {}
    void draw(VkCommandBuffer cb) {}
    rc_id get_rc_id() const { return 0; }
    utils::shading_type get_shading_type() const { return type; }
  private:
    utils::shading_type type = T;
};

}} // namespace hnll::game