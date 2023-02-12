#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <utils/common_alias.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/singleton.hpp>

// lib
#include <GLFW/glfw3.h>

namespace hnll {

namespace game {

class gui_engine;

// common impl
// should be wrapped by utils::singleton
class engine_core
{
  public:
    engine_core(const std::string &application_name, utils::rendering_type rendering_type);
    void render_gui();
  private:
    // glfw
    static void set_glfw_mouse_button_callbacks();
    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
    static void add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow *, int, int, int)>> &&func);
    static std::vector<u_ptr<std::function<void(GLFWwindow *, int, int, int)>>> glfw_mouse_button_callbacks_;

    static graphics_engine_core& graphics_engine_core_;
    static u_ptr<gui_engine> gui_engine_;
};

// parametric impl
// for multiple template parameter packs
template <ShadingSystem... T> struct shading_system_list {};
template <Actor... A>         struct actor_list {};
template <typename... T> class engine;

template <ShadingSystem... S, Actor... A>
class engine<shading_system_list<S...>, actor_list<A...>>
{
    using actor_map = std::unordered_map<uint32_t, std::variant<u_ptr<A>...>>;
  public:
    engine(const std::string& application_name = "honolulu engine", utils::rendering_type rendering_type = utils::rendering_type::VERTEX_SHADING);
    ~engine();

    void run();

  private:
    void update();
    void render();

    // for each specific engine
    void update_this() {}

    // common part
    engine_core& core_;
    graphics_engine_core& graphics_engine_core_;

    // parametric part
    u_ptr<graphics_engine<S...>> graphics_engine_;
    actor_map actors_;
};

// impl
#define ENGN_API template <ShadingSystem... S, Actor... A>
#define ENGN_TYPE engine<shading_system_list<S...>, actor_list<A...>>

ENGN_API ENGN_TYPE::engine(const std::string &application_name, utils::rendering_type rendering_type)
{
  core_ = utils::singleton<engine_core>::get_instance(application_name, rendering_type);
  graphics_engine_core_ = utils::singleton<graphics_engine_core>::get_instance(application_name, rendering_type);

  graphics_engine_ = graphics_engine<S...>::create();
}

ENGN_API void ENGN_TYPE::run()
{
  while (!graphics_engine_core_.should_close_window()) {
    glfwPollEvents();
    update();
    render();
  }
  graphics_engine_core_.wait_idle();
}

ENGN_API void ENGN_TYPE::update()
{ update_this(); for (auto& a : actors_) a->update(); }

ENGN_API void ENGN_TYPE::render()
{ graphics_engine_->render(); core_.render_gui(); }

}} // namespace hnll::game