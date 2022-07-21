// this class organizes all the graphics specific features
// hnll
#include <graphics/engine.hpp>
#include <graphics/systems/point_light.hpp>
#include <graphics/systems/mesh_rendering_system.hpp>
#include <graphics/systems/line_rendering_system.hpp>

//std
#include <stdexcept>
#include <array>
#include <iostream>

// lib
#include <glm/gtc/constants.hpp>

namespace hnll {
namespace graphics {

engine::engine(const char* window_name) : window_{WIDTH, HEIGHT, window_name}
{ init(); }

engine::~engine()
{ }

// todo : separate into some functions
void engine::init()
{
  // // 2 uniform buffer descriptor
  global_pool_ = descriptor_pool::builder(device_)
    .set_max_sets(swap_chain::MAX_FRAMES_IN_FLIGHT)
    .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, swap_chain::MAX_FRAMES_IN_FLIGHT)
    .build();

  // creating ubo for each frames version
  for (int i = 0; i < ubo_buffers_.size(); i++) {
    ubo_buffers_[i] = std::make_unique<buffer>(
      device_,
      sizeof(global_ubo),
      1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    );
    ubo_buffers_[i]->map();
  }

  // this is set layout of master system
  // enable ubo to be referenced by oall stages of a graphics pipeline
  global_set_layout_ = descriptor_set_layout::builder(device_)
    .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
    .build();
  // may add additional layout of child system

  for (int i = 0; i < global_descriptor_sets_.size(); i++) {
    auto bufferInfo = ubo_buffers_[i]->descriptor_info();
    descriptor_writer(*global_set_layout_, *global_pool_)
      .write_buffer(0, &bufferInfo)
      .build(global_descriptor_sets_[i]);
  }

  // create renderer system as local variable
  auto mesh_renderer = std::make_unique<mesh_rendering_system>(
    device_, 
    renderer_.get_swap_chain_render_pass(HVE_RENDER_PASS_ID),
    global_set_layout_->get_descriptor_set_layout()
  );

  auto point_light_renderer = std::make_unique<point_light_rendering_system>(
    device_, 
    renderer_.get_swap_chain_render_pass(HVE_RENDER_PASS_ID),
    global_set_layout_->get_descriptor_set_layout()
  );

//  auto line_renderer = std::make_unique<line_rendering_system>(
//    device_,
//    renderer_.get_swap_chain_render_pass(HVE_RENDER_PASS_ID),
//    global_set_layout_->get_descriptor_set_layout()
//  );

  rendering_systems_.emplace
    (mesh_renderer->get_render_type(), std::move(mesh_renderer));
  rendering_systems_.emplace
    (point_light_renderer->get_render_type(), std::move(point_light_renderer));
//  rendering_systems_.emplace
//    (line_renderer->get_render_type(), std::move(line_renderer));
}

// each render systems automatically detect render target components
void engine::render(hnll::game::viewer_component& viewer_comp)
{
  // returns nullptr if the swap chain is need to be recreated
  if (auto command_buffer = renderer_.begin_frame()) {
    int frame_index = renderer_.get_frame_index();

    frame_info frame_info{
        frame_index, 
        command_buffer, 
        global_descriptor_sets_[frame_index]
    };

    // update 
    ubo_.projection = viewer_comp.get_projection().cast<float>();
    ubo_.view = viewer_comp.get_view().cast<float>();
    ubo_buffers_[frame_index]->write_to_buffer(&ubo_);
    ubo_buffers_[frame_index]->flush();

    // rendering
    // TODO : configure hve_render_pass_id as the 
    // member and detect it in begin_swap_chain_render_pass func
    renderer_.begin_swap_chain_render_pass(command_buffer, HVE_RENDER_PASS_ID);
    // programmable stage of rendering
    // system can now access gameobjects via frame_info
    for (auto& system : rendering_systems_)
      system.second->render(frame_info);

    renderer_.end_swap_chain_render_pass(command_buffer);
    renderer_.end_frame();
  }
}

void engine::remove_renderable_component_without_owner(hnll::game::render_type type, hnll::game::component_id id)
{
  rendering_systems_[type]->remove_render_target(id);
}

} // namespace graphics
} // namespace hnll