#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <utils/common_alias.hpp>
#include <utils/rendering_utils.hpp>

// lib
#include <GLFW/glfw3.h>

namespace hnll {

namespace game {

class gui_engine;

class engine
{
  public:
    engine(const std::string& application_name = "honolulu engine", utils::rendering_type rendering_type = utils::rendering_type::VERTEX_SHADING);
    ~engine();

    void run();

  private:
    void update();
    void render();

    // glfw
    static void set_glfw_mouse_button_callbacks();
    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
    static void add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow *, int, int, int)>> &&func);
    static std::vector<u_ptr<std::function<void(GLFWwindow *, int, int, int)>>> glfw_mouse_button_callbacks_;

    // modules
    static u_ptr<graphics_engine> graphics_engine_;
    u_ptr<gui_engine> gui_engine_;
};

}} // namespace hnll::game