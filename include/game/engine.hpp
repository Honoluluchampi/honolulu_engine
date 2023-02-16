#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/graphics_model.hpp>
#include <utils/common_alias.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/singleton.hpp>

// std
#include <chrono>

// lib
#include <GLFW/glfw3.h>

// macro for crtp
#define DEFINE_ENGINE(new_engine, shading_systems, actors) class new_engine : public game::engine_base<new_engine, shading_systems, actors>
#define SELECT_SHADING_SYSTEM(name, ...) using name = game::shading_system_list<__VA_ARGS__>
#define SELECT_ACTOR(name, ...) using name = game::actor_list<__VA_ARGS__>

namespace hnll {

namespace game {

class gui_engine;

// model name -> model
template <utils::shading_type type>
using graphics_model_map = std::unordered_map<std::string, u_ptr<graphics::graphics_model<type>>>;

// common impl
// should be wrapped by utils::singleton
class engine_core
{
  public:
    engine_core(const std::string &application_name, utils::rendering_type rendering_type);
    ~engine_core();

    void render_gui();

    inline const std::chrono::system_clock::time_point& get_old_time() const { return old_time_; };
    inline void set_old_time(std::chrono::system_clock::time_point&& time) { old_time_ = std::move(time); }

    inline const utils::viewer_info& get_viewer_info() const { return viewer_info_; }
    static inline void set_viewer_info(utils::viewer_info&& v) { viewer_info_ = std::move(v); }

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

    std::chrono::system_clock::time_point old_time_;
    static utils::viewer_info viewer_info_;
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

    template <Actor Act>
    static void add_update_target(u_ptr<Act>&& target);

    template <Actor Act, typename... Args>
    static void add_update_target_directly(Args&&... args);

    template <ShadingSystem SS, RenderableComponent RC>
    void add_render_target(RC& rc)
    { graphics_engine_->template add_render_target<SS>(rc); }

  private:
    void update();
    void render();
    void cleanup();

    // for each specific engine
    void update_this(const float& dt) {}

    // common part
    engine_core& core_;
    graphics_engine_core& graphics_engine_core_;

    // parametric part
    u_ptr<graphics_engine<S...>> graphics_engine_;
    static actor_map update_target_actors_;
};

// impl
#define ENGN_API template <class Derived, ShadingSystem... S, Actor... A>
#define ENGN_TYPE engine_base<Derived, shading_system_list<S...>, actor_list<A...>>

// static members
ENGN_API std::unordered_map<uint32_t, std::variant<u_ptr<A>...>> ENGN_TYPE::update_target_actors_;

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
    update();
    render();
  }
  graphics_engine_core_.wait_idle();
  cleanup();
}

ENGN_API void ENGN_TYPE::update()
{
  // calc delta time
  std::chrono::system_clock::time_point new_time = std::chrono::system_clock::now();;
  const auto& old_time = core_.get_old_time();
  auto dt = std::chrono::duration<float, std::chrono::seconds::period>(new_time - old_time).count();
  core_.set_old_time(std::move(new_time));

  static_cast<Derived*>(this)->update_this(dt);
  for (auto& a : update_target_actors_)
    std::visit([&dt](auto& actor) { actor->update(dt); }, a.second);
}

ENGN_API void ENGN_TYPE::render()
{
  graphics_engine_->render(core_.get_viewer_info());
  core_.render_gui();
}

ENGN_API void ENGN_TYPE::cleanup()
{
  graphics_engine_.reset();
  utils::singleton_deleter::delete_reversely();
}

ENGN_API template <Actor Act> void ENGN_TYPE::add_update_target(u_ptr<Act>&& target)
{ update_target_actors_[target->get_actor_id()] = std::move(target); }

ENGN_API template <Actor Act, typename... Args> void ENGN_TYPE::add_update_target_directly(Args &&...args)
{
  auto target = Act::create(std::forward<Args>(args)...);
  update_target_actors_[target->get_actor_id()] = std::move(target);
}


}} // namespace hnll::game