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
    ~engine_core();

    void render_gui();

  private:
    void update_gui() {}

    void cleanup();

    // glfw
    void set_glfw_mouse_button_callbacks();
    void add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow *, int, int, int)>> &&func);
    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
    static std::vector<u_ptr<std::function<void(GLFWwindow *, int, int, int)>>> glfw_mouse_button_callbacks_;

    graphics_engine_core& graphics_engine_core_;
    static u_ptr<gui_engine> gui_engine_;
};

// parametric impl
// for multiple template parameter packs
template <ShadingSystem... T> struct shading_system_list {};
template <Actor... A>         struct actor_list {};
template <class Derived, typename... T> class engine_base;

template <class Derived, ShadingSystem... S, Actor... A>
class engine_base<Derived, shading_system_list<S...>, actor_list<A...>>
{
    using actor_map = std::unordered_map<uint32_t, std::variant<u_ptr<A>...>>;
  public:
    engine_base(const std::string& application_name = "honolulu engine", utils::rendering_type rendering_type = utils::rendering_type::VERTEX_SHADING);
    ~engine_base(){}

    void run();

  private:
    void update(const float& dt);
    void render();
    void cleanup();

    // for each specific engine
    void update_this(const float& dt) {}

    // common part
    engine_core& core_;
    graphics_engine_core& graphics_engine_core_;

    // parametric part
    u_ptr<graphics_engine<S...>> graphics_engine_;
    actor_map actors_;
};

// impl
#define ENGN_API template <class Derived, ShadingSystem... S, Actor... A>
#define ENGN_TYPE engine_base<Derived, shading_system_list<S...>, actor_list<A...>>

ENGN_API ENGN_TYPE::engine_base(const std::string &application_name, utils::rendering_type rendering_type)
 : core_(utils::singleton<engine_core>::get_instance(application_name, rendering_type)),
   graphics_engine_core_(utils::singleton<graphics_engine_core>::get_instance(application_name, rendering_type))
{
  graphics_engine_ = graphics_engine<S...>::create(application_name, rendering_type);
}

ENGN_API void ENGN_TYPE::run()
{
  while (!graphics_engine_core_.should_close_window()) {
    glfwPollEvents();
    update(0.1);
    render();
  }
  graphics_engine_core_.wait_idle();
  cleanup();
}

ENGN_API void ENGN_TYPE::update(const float& dt)
{
  static_cast<Derived*>(this)->update_this(dt);
  for (auto& a : actors_)
    std::visit([&dt](auto& actor) { actor->update(dt); }, a.second);
}

ENGN_API void ENGN_TYPE::render()
{ graphics_engine_->render(); core_.render_gui(); }

ENGN_API void ENGN_TYPE::cleanup()
{
  graphics_engine_.reset();
  utils::singleton_deleter::delete_reversely();
}

}} // namespace hnll::game