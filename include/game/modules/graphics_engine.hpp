#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <map>
#include <vector>

namespace hnll {

namespace graphics {
  class window;
  class device;
  class renderer;
  class swap_chain;
  class descriptor_set_layout;
  class descriptor_pool;
  class buffer;
}

namespace game {

// forward declaration
class shading_system;
class renderable_component;

class graphics_engine
{
  using shading_system_map = std::map<uint32_t, u_ptr<shading_system>>;
  public:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 820;
    static constexpr float MAX_FRAME_TIME = 0.05;

    graphics_engine(const char* window_name);
    ~graphics_engine();

    static u_ptr<graphics_engine> create(const char* window_name = "honolulu engine")
    { return std::make_unique<graphics_engine>(window_name); }

    graphics_engine(const graphics_engine &) = delete;
    graphics_engine &operator= (const graphics_engine &) = delete;

    void render();

    // getter
    bool should_close_window();

  private:
    void init();

    // construct in impl
    u_ptr<graphics::window>   window_;
};

}
} // namespace hnll::graphics
