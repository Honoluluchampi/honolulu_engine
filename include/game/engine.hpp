#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <game/modules/compute_engine.hpp>
#include <graphics/graphics_model.hpp>
#include <utils/common_alias.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/singleton.hpp>

// std
#include <chrono>

// lib
#include <GLFW/glfw3.h>

// macro for crtp
#define DEFINE_ENGINE(name) class name : public game::engine_base<name, selected_shading_systems, selected_actors, selected_compute_shaders>
#define SELECT_SHADING_SYSTEM(...) using selected_shading_systems = game::shading_system_list<__VA_ARGS__>
#define SELECT_ACTOR(...)          using selected_actors          = game::actor_list<__VA_ARGS__>
#define SELECT_COMPUTE_SHADER(...) using selected_compute_shaders = game::compute_shader_list<__VA_ARGS__>
#define ENGINE_CTOR(name) name(const std::string& app_name = "app", hnll::utils::rendering_type type = hnll::utils::rendering_type::VERTEX_SHADING) \
                          : game::engine_base<name, selected_shading_systems, selected_actors, selected_compute_shaders>(app_name, type)

namespace hnll {

namespace game {

class gui_engine;

// model name -> model
template <graphics::GraphicsModel M>
using graphics_model_map = std::unordered_map<std::string, u_ptr<M>>;

// common impl
// should be wrapped by utils::singleton
class engine_core
{
  public:
    engine_core(const std::string &application_name, utils::rendering_type rendering_type);
    ~engine_core();

    void render_gui();

    void begin_imgui();

    float get_dt();
    inline const std::chrono::system_clock::time_point& get_old_time() const { return old_time_; };
    inline void set_old_time(std::chrono::system_clock::time_point&& time) { old_time_ = std::move(time); }

    inline const utils::viewer_info& get_viewer_info() const { return viewer_info_; }
    static inline void set_viewer_info(utils::viewer_info&& v) { viewer_info_ = std::move(v); }

    float get_max_fps() const { return max_fps_; }
    void set_max_fps(float max_fps) { max_fps_ = max_fps; }

    // glfw
    vec2 get_cursor_pos() const;
    static void add_glfw_mouse_button_callback(std::function<void(GLFWwindow *, int, int, int)> &&func);

  private:
    void update_gui() {}

    void cleanup();

    // glfw
    void set_glfw_callbacks();
    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
    static std::vector<std::function<void(GLFWwindow *, int, int, int)>> glfw_mouse_button_callbacks_;

    graphics_engine_core& graphics_engine_core_;
#ifndef IMGUI_DISABLED
    gui_engine& gui_engine_;
#endif
    std::chrono::system_clock::time_point old_time_;
    static utils::viewer_info viewer_info_;
    float max_fps_ = 60.f;
};

// parametric impl
// for multiple template parameter packs
template <ShadingSystem... T> struct shading_system_list {};
template <Actor... A>         struct actor_list {};
template <ComputeShader... C> struct compute_shader_list {};
using no_actor = actor_list<>;
template <class Derived, typename... T> class engine_base;

template <class Derived, ShadingSystem... S, Actor... A, ComputeShader... C>
class engine_base<Derived, shading_system_list<S...>, actor_list<A...>, compute_shader_list<C...>>
{
    using actor_map = std::unordered_map<uint32_t, std::variant<u_ptr<A>...>>;
  public:
    engine_base(const std::string& application_name = "honolulu engine", utils::rendering_type rendering_type = utils::rendering_type::VERTEX_SHADING);
    virtual ~engine_base();

    void run();

    template <Actor Act>
    static void add_update_target(u_ptr<Act>&& target);

    template <Actor Act, typename... Args>
    static void add_update_target_directly(Args&&... args);

    template <ShadingSystem SS, RenderableComponent RC>
    inline void add_render_target(RC& rc)
    { graphics_engine_->template add_render_target<SS>(rc); }

    inline float get_max_fps() const { return core_.get_max_fps(); }

  protected:
    // cleaning method of each specific application
    void cleanup() {}
    inline void set_max_fps(float max_fps) { core_.set_max_fps(max_fps); }

    engine_core& core_;

  private:
    void update();
    void render();

    // for each specific engine
    void update_this(const float& dt) {}

    // common part
    graphics_engine_core& graphics_engine_core_;

    float dt_;

    // parametric part
    u_ptr<graphics_engine<S...>> graphics_engine_;
    u_ptr<compute_engine<C...>> compute_engine_;

    static actor_map update_target_actors_;
};

// impl
#define ENGN_API template <class Derived, ShadingSystem... S, Actor... A, ComputeShader... C>
#define ENGN_TYPE engine_base<Derived, shading_system_list<S...>, actor_list<A...>, compute_shader_list<C...>>

// static members
ENGN_API typename ENGN_TYPE::actor_map ENGN_TYPE::update_target_actors_;

ENGN_API ENGN_TYPE::engine_base(const std::string &application_name, utils::rendering_type rendering_type)
 : graphics_engine_core_(utils::singleton<graphics_engine_core>::get_instance(application_name, rendering_type)),
   core_(utils::singleton<engine_core>::get_instance(application_name, rendering_type))
{
  graphics_engine_ = graphics_engine<S...>::create(application_name, rendering_type);

  // only if any compute shader is defined
  if constexpr (sizeof...(C) >= 1) {
    compute_engine_ = compute_engine<C...>::create(
      graphics_engine_core_.get_compute_semaphore_r()
    );
  }
}

ENGN_API ENGN_TYPE::~engine_base()
{
  static_cast<Derived*>(this)->cleanup();
  graphics_engine_.reset();
  compute_engine_.reset();
  utils::singleton_deleter::delete_reversely();
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
{
  // calc delta time
  dt_ = core_.get_dt();

  core_.begin_imgui();

  static_cast<Derived*>(this)->update_this(dt_);
  if constexpr (sizeof...(A) >= 1) {
    for (auto &a: update_target_actors_)
      std::visit([this](auto &actor) { actor->update(this->dt_); }, a.second);
  }
}

ENGN_API void ENGN_TYPE::render()
{
  if constexpr (sizeof...(C) >= 1)
    compute_engine_->render(dt_);

  utils::game_frame_info game_frame_info = { 0, core_.get_viewer_info() };
  graphics_engine_->render(game_frame_info);
  core_.render_gui();
}

ENGN_API template <Actor Act> void ENGN_TYPE::add_update_target(u_ptr<Act>&& target)
{ update_target_actors_[target->get_actor_id()] = std::move(target); }

ENGN_API template <Actor Act, typename... Args> void ENGN_TYPE::add_update_target_directly(Args &&...args)
{
  auto target = Act::create(std::forward<Args>(args)...);
  update_target_actors_[target->get_actor_id()] = std::move(target);
}


}} // namespace hnll::game