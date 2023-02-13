#pragma once

#pragma once

// hnll
#include <game/component.hpp>
#include <utils/utils.hpp>

// std
#include <unordered_map>
#include <functional>

// lib
#include <GLFW/glfw3.h>

namespace hnll::game {

struct key_mappings;
struct pad_mappings;

DEFINE_COMPONENT(key_move_comp)
{
  public:
    using key_id = int;
    using pad_id = int;
    using button_map = std::unordered_map<key_id, u_ptr<std::function<void()>>>;
    using pad_map = std::unordered_map<pad_id, u_ptr<std::function<vec3(float,float)>>>;

    key_move_comp(GLFWwindow* window, hnll::utils::transform& transform);

    enum class updating { ON, OFF };

    // update owner's position (owner's transformation's ref was passed in the ctor)
    void update(const float& dt);

    // dont use lambda's capture
    void set_button_func(key_id key_id, std::function<void()> func)
    { button_map_.emplace(key_id, std::make_unique<std::function<void()>>(func)); }
    // dont use lambda's capture
    void set_axis_func(pad_id axis_id, std::function<vec3(float, float)> func)
    { pad_map_.emplace(axis_id, std::make_unique<std::function<vec3(float, float)>>(func)); }

    void set_updating_on()  { updating_ = updating::ON; }
    void set_updating_off() { updating_ = updating::OFF; }

    // getter
    bool is_updating() const { return updating_ == updating::ON; }

    void remove_button_func(key_id key_id) { button_map_.erase(key_id); }
    void remove_axis_func(pad_id axis_id) { pad_map_.erase(axis_id); }

  private:
    void adjust_axis_errors();
    void set_default_mapping();
    void process_rotate_input(GLFWgamepadstate& state, float dt);
    void process_move_input(GLFWgamepadstate& state, float dt);
    void process_button_input(GLFWgamepadstate& state, float dt);

    // mapping
    static key_mappings keys;
    static pad_mappings pads;

    updating updating_ = updating::ON;

    GLFWwindow* window_;
    // this component should be deleted before the owner
    hnll::utils::transform& transform_;

    button_map button_map_;
    // funcs take GLFWgamepadstate.axes[glfw_gamepad_axis_leftorright_xory], dt
    pad_map pad_map_;
    int pad_number_ = GLFW_JOYSTICK_1;
    // to adjust default input
    float right_error_ = 0.f, up_error_ = 0.f;
};

} // namespace hnll::game