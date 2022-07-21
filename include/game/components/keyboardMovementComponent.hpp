#pragma once

// hnll
#include <game/component.hpp>
#include <utils/utils.hpp>

// std
#include <unordered_map>
#include <functional>

// lib
#include <GLFW/glfw3.h>

namespace hnll {
namespace game {

class keyboard_movement_component : public component
{
  public:
    using key_id = int;
    using pad_id = int;
    using button_map = std::unordered_map<key_id, u_ptr<std::function<void()>>>;
    using pad_map = std::unordered_map<pad_id, u_ptr<std::function<Eigen::Vector3f(float,float)>>>;
    
    keyboard_movement_component(GLFWwindow* window, hnll::utils::transform& transform);
    
    struct key_mappings 
    {
      key_id move_left = GLFW_KEY_A;
      key_id move_right = GLFW_KEY_D;
      key_id move_forward = GLFW_KEY_W;
      key_id move_backward = GLFW_KEY_S;
      key_id move_up = GLFW_KEY_E;
      key_id move_down = GLFW_KEY_Q;
      key_id look_left = GLFW_KEY_LEFT;
      key_id look_right = GLFW_KEY_RIGHT;
      key_id look_up = GLFW_KEY_UP;
      key_id look_down = GLFW_KEY_DOWN;
    };

    struct pad_mappings
    {
      pad_id button_a = GLFW_GAMEPAD_BUTTON_A;
      pad_id button_b = GLFW_GAMEPAD_BUTTON_B;
      pad_id button_y = GLFW_GAMEPAD_BUTTON_Y;
      pad_id button_x = GLFW_GAMEPAD_BUTTON_X;
      pad_id button_guide = GLFW_GAMEPAD_BUTTON_GUIDE;
      pad_id button_start = GLFW_GAMEPAD_BUTTON_START;
      pad_id left_bumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
      pad_id right_bumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
      pad_id dp_up = GLFW_GAMEPAD_BUTTON_DPAD_UP;
      pad_id dp_down = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
      pad_id move_x = GLFW_GAMEPAD_AXIS_LEFT_X;
      pad_id move_y = GLFW_GAMEPAD_AXIS_LEFT_Y;
      pad_id rota_x = GLFW_GAMEPAD_AXIS_RIGHT_Y;
      pad_id rota_y = GLFW_GAMEPAD_AXIS_RIGHT_X;
    };

    // update owner's position (owner's transformation's ref was passed in the ctor)
    void update_component(float dt) override;

    // dont use lambda's capture
    // TODO : check whether using checkingButtonList fasten the checking sequence
    void set_button_func(key_id key_id, std::function<void()> func)
    { button_map_.emplace(key_id, std::make_unique<std::function<void()>>(func)); }
    // dont use lambda's capture
    void set_axis_func(pad_id axis_id, std::function<Eigen::Vector3f(float, float)> func)
    { pad_map_.emplace(axis_id, std::make_unique<std::function<Eigen::Vector3f(float, float)>>(func)); }

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

} // namespace game
} // namespace hnll