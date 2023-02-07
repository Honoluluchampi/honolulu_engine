// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::game {

u_ptr<graphics_engine> graphics_engine::create(const std::string& window_name, utils::rendering_type rendering_type)
{ return std::make_unique<graphics_engine>(window_name, rendering_type);}

graphics_engine::graphics_engine(const std::string& window_name, utils::rendering_type rendering_type)
{
  window_ = graphics::window::create(WIDTH, HEIGHT, window_name);
  device_ = graphics::device::create(*window_, rendering_type);
  init();
}

// delete static vulkan objects explicitly, because static member would be deleted after non-static member(device)
graphics_engine::~graphics_engine()
{

}

// todo : separate into some functions
void graphics_engine::init()
{
}

// getter
bool graphics_engine::should_close_window() { return window_->should_be_closed(); }

} // namespace hnll::game