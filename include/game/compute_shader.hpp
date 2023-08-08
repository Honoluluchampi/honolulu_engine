#pragma once

// hnll
#include <graphics/device.hpp>
#include <graphics/compute_pipeline.hpp>
#include <utils/utils.hpp>
#include <utils/singleton.hpp>
#include <utils/frame_info.hpp>

// lib
#include <vulkan/vulkan.h>

#define DEFINE_COMPUTE_SHADER(new_shader) class new_shader : public game::compute_shader<new_shader>
#define DEFAULT_COMPUTE_SHADER_CTOR(new_shader) explicit new_shader()
#define DEFAULT_COMPUTE_SHADER_CTOR_IMPL(shader) shader::shader() : game::compute_shader<shader>() { }

namespace hnll{

namespace graphics { class desc_layout; }

namespace game {

template <typename Derived>
class compute_shader
  {
    public:
      explicit compute_shader() : device_(utils::singleton<graphics::device>::get_instance())
      { static_cast<Derived*>(this)->setup(); }
      static u_ptr<Derived> create() { return std::make_unique<Derived>(); }

      virtual ~compute_shader(){}

      // for physics simulation
      void render(const utils::compute_frame_info& info);

    protected:
      // write this function for each compute shader
      void setup();

      template<typename PushConstant>
      u_ptr<graphics::compute_pipeline> create_pipeline(
        const std::string& filepath,
        const std::vector<VkDescriptorSetLayout>& desc_set_layouts
      );

      // compute command dispatching
      inline void bind_pipeline(VkCommandBuffer command)
      { vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_->get_vk_pipeline()); }

      inline void bind_desc_sets(VkCommandBuffer command, const std::vector<VkDescriptorSet>& desc_sets)
      { vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_->get_vk_layout(), 0, desc_sets.size(), desc_sets.data(), 0, 0); }

      inline void dispatch_command(VkCommandBuffer command, int x, int y, int z)
      { vkCmdDispatch(command, x, y, z); }

      template <typename PushConstant>
      inline void bind_push(VkCommandBuffer command, VkShaderStageFlagBits stages, PushConstant push)
      { vkCmdPushConstants(command, pipeline_->get_vk_layout(), stages, 0, sizeof(PushConstant), &push); }

      // default objects
      u_ptr<graphics::compute_pipeline> pipeline_;

      graphics::device& device_;
      u_ptr<graphics::desc_layout> desc_layout_ = nullptr;
  };

#define CS_API template <typename Derived>
#define CS_TYPE compute_shader<Derived>

CS_API template <typename PushConstant> u_ptr<graphics::compute_pipeline> CS_TYPE::create_pipeline(
  const std::string &filepath,
  const std::vector<VkDescriptorSetLayout> &desc_set_layouts)
{ return graphics::compute_pipeline::create(device_, desc_set_layouts, filepath, sizeof(PushConstant)); }
}} // namespace hnll::game