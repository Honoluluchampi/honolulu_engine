// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>

namespace hnll::game {

graphics_engine::graphics_engine(const char* window_name)
{
  window_ = graphics::window::create(WIDTH, HEIGHT, window_name);

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