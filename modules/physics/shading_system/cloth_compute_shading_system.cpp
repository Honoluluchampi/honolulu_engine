// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>

namespace hnll::physics {

std::unordered_map<uint32_t, s_ptr<mass_spring_cloth>> cloth_compute_shading_system::clothes_;

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(cloth_compute_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>);
cloth_compute_shading_system::~cloth_compute_shading_system()
{
  clothes_.clear();
  mass_spring_cloth::reset_desc_layout();
}

void cloth_compute_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESH;

  mass_spring_cloth::set_desc_layout();

  pipeline_layout_ = create_pipeline_layout_without_push(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{
      game::graphics_engine_core::get_global_desc_layout(),
      mass_spring_cloth::get_vk_desc_layout()
    }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    game::graphics_engine_core::get_default_render_pass(),
    // TODO : configure different shader directories for each shaders
    std::vector<std::string>{ "/modules/physics/shaders/spv/", "/modules/graphics/shaders/spv/" },
    { "cloth_compute.vert.spv", "simple_shader.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void cloth_compute_shading_system::render(const utils::graphics_frame_info& frame_info)
{
  pipeline_->bind(frame_info.command_buffer);

  for (auto& cloth_kv : clothes_) {
    auto& cloth = cloth_kv.second;

    std::vector<VkDescriptorSet> desc_sets = {
      frame_info.global_descriptor_set,
      cloth->get_vk_desc_sets(frame_info.frame_index)[0]
    };

    vkCmdBindDescriptorSets(
      frame_info.command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline_layout_,
      0,
      desc_sets.size(),
      desc_sets.data(),
      0,
      nullptr
    );

    vkCmdDraw(frame_info.command_buffer, 3, 1, 0, 0);
  }
}

void cloth_compute_shading_system::add_cloth(const s_ptr<mass_spring_cloth>& cloth)
{ clothes_[cloth->get_id()] = cloth; }

void cloth_compute_shading_system::remove_cloth(uint32_t cloth_id)
{ clothes_.erase(cloth_id); }

} // namespace hnll::physics