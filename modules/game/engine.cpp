// hnll
#include <game/engine.hpp>

namespace hnll::game {

void engine::run() {
  while (!graphics_engine_->should_close_window()) {
    update();
    render();
  }
}

void engine::update() {

}

void engine::render() {

}

} // namespace hnll::game