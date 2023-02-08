// hnll
#include <game/engine.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <iostream>

namespace hnll::game {

engine::engine(const std::string& application_name, utils::rendering_type rendering_type)
{
  graphics_engine_ = graphics_engine::create(application_name, rendering_type);
}

void engine::run()
{
  while (!graphics_engine_->should_close_window()) {
    update();
    render();
  }
}

void engine::update()
{

}

void engine::render()
{
  //graphics_engine_->render();
}

} // namespace hnll::game