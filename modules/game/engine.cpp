// hnll
#include <game/engine.hpp>

// std
#include <iostream>

namespace hnll::game {

engine::engine()
{
  graphics_engine_ = graphics_engine::create();
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

}

} // namespace hnll::game