// hnll
#include <game/engine.hpp>
#include <game/components/key_move_comp.hpp>

void joystickCallback(int jid, int event)
{
  std::cout << jid << std::endl;
  if (event == GLFW_CONNECTED) std::cout << "connected" << std::endl;
  else std::cout << "disconnected" << std::endl;
}

namespace hnll::game {

constexpr float MOVE_THRESH = 0.1f;
constexpr float ROTATE_THRESH = 0.1f;

constexpr float MOVE_SPEED = 5.f;
constexpr float LOOK_SPEED = 1.5f;
constexpr float CURSOR_SPEED = 15.f;

struct key_mappings
{
  key_move_comp::key_id move_left     = GLFW_KEY_A;
  key_move_comp::key_id move_right    = GLFW_KEY_D;
  key_move_comp::key_id move_forward  = GLFW_KEY_W;
  key_move_comp::key_id move_backward = GLFW_KEY_S;
  key_move_comp::key_id move_up       = GLFW_KEY_E;
  key_move_comp::key_id move_down     = GLFW_KEY_Q;
  key_move_comp::key_id look_left     = GLFW_KEY_LEFT;
  key_move_comp::key_id look_right    = GLFW_KEY_RIGHT;
  key_move_comp::key_id look_up       = GLFW_KEY_UP;
  key_move_comp::key_id look_down     = GLFW_KEY_DOWN;
};

struct pad_mappings
{
  key_move_comp::pad_id button_a     = GLFW_GAMEPAD_BUTTON_A;
  key_move_comp::pad_id button_b     = GLFW_GAMEPAD_BUTTON_B;
  key_move_comp::pad_id button_y     = GLFW_GAMEPAD_BUTTON_Y;
  key_move_comp::pad_id button_x     = GLFW_GAMEPAD_BUTTON_X;
  key_move_comp::pad_id button_guide = GLFW_GAMEPAD_BUTTON_GUIDE;
  key_move_comp::pad_id button_start = GLFW_GAMEPAD_BUTTON_START;
  key_move_comp::pad_id left_bumper  = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
  key_move_comp::pad_id right_bumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
  key_move_comp::pad_id dp_up        = GLFW_GAMEPAD_BUTTON_DPAD_UP;
  key_move_comp::pad_id dp_down      = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
  key_move_comp::pad_id move_x       = GLFW_GAMEPAD_AXIS_LEFT_X;
  key_move_comp::pad_id move_y       = GLFW_GAMEPAD_AXIS_LEFT_Y;
  key_move_comp::pad_id rota_x       = GLFW_GAMEPAD_AXIS_RIGHT_Y;
  key_move_comp::pad_id rota_y       = GLFW_GAMEPAD_AXIS_RIGHT_X;
};

key_mappings key_move_comp::keys{};
pad_mappings key_move_comp::pads{};

key_move_comp::key_move_comp(GLFWwindow* window, hnll::utils::transform& transform)
  : window_(window), transform_(transform)
{
  // mapping
  const char* mapping = "03000000790000004618000010010000,Nintendo GameCube Controller Adapter,a:b1,b:b2,dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,lefttrigger:b4,leftx:a0,lefty:a1,leftshoulder:b4,rightshoulder:b5,guide:b7,rightx:a5~,righty:a2~,start:b9,x:b0,y:b3,platform:Linux";
  glfwUpdateGamepadMappings(mapping);
  glfwSetJoystickCallback(joystickCallback);
  adjust_axis_errors();
}

void key_move_comp::update(const float& dt)
{
  if (updating_ == updating::OFF) return;
  GLFWgamepadstate state;
  glfwGetGamepadState(pad_number_, &state);
  process_rotate_input(state, dt);
  process_move_input(state,dt);
  process_button_input(state, dt);
}

void key_move_comp::process_rotate_input(GLFWgamepadstate& state, float dt)
{
  float rota_x = state.axes[pads.rota_x]
                 + (glfwGetKey(window_, keys.look_up) == GLFW_PRESS)
                 - (glfwGetKey(window_, keys.look_down) == GLFW_PRESS);
  float rota_y = -state.axes[pads.rota_y]
                 + (glfwGetKey(window_, keys.look_right) == GLFW_PRESS)
                 - (glfwGetKey(window_, keys.look_left) == GLFW_PRESS);
  vec3 rotate = {rota_x, rota_y, 0.f};
  // add to the current rotate matrix
  if (rotate.dot(rotate) > ROTATE_THRESH) //std::numeric_limits<float>::epsilon())
    transform_.rotation += LOOK_SPEED * dt * rotate.normalized();

  // limit pitch values between about +/- 58ish degrees
  transform_.rotation.x() = std::clamp(transform_.rotation.x(), -1.5f, 1.5f);
  transform_.rotation.y() = std::fmod(transform_.rotation.y(), M_PI * 2.f);
}

void key_move_comp::process_move_input(GLFWgamepadstate& state, float dt)
{
  // camera translation
  // store user input
  float move_x = state.axes[pads.move_x] - right_error_;
  float move_y = -(state.axes[pads.move_y] - up_error_);
  float moveZ = state.buttons[pads.dp_up] - state.buttons[pads.dp_down];

  float yaw = transform_.rotation.y();
  const vec3 forward_direction{sin(yaw), 0.f, cos(yaw)};
  const vec3 right_direction{forward_direction.z(), 0.f, -forward_direction.x()};
  const vec3 up_direction{0.f, -1.f, 0.f};

  vec3 move_direction { 0.f, 0.f, 0.f };
  float forward = move_y * (state.buttons[pads.left_bumper] == GLFW_PRESS)
                  + (glfwGetKey(window_, keys.move_forward) == GLFW_PRESS)
                  - (glfwGetKey(window_, keys.move_backward) == GLFW_PRESS);
  float right = move_x * (state.buttons[pads.left_bumper] == GLFW_PRESS)
                + (glfwGetKey(window_, keys.move_right) == GLFW_PRESS)
                - (glfwGetKey(window_, keys.move_left) == GLFW_PRESS);
  float up = (glfwGetKey(window_, keys.move_up) == GLFW_PRESS) - (glfwGetKey(window_, keys.move_down) == GLFW_PRESS)
             + moveZ;

  move_direction += forward * forward_direction + right * right_direction + up * up_direction;

  if (move_direction.dot(move_direction) > std::numeric_limits<float>::epsilon())
    transform_.translation += MOVE_SPEED * dt * move_direction.normalized();

  // cursor move
  double xpos, ypos;
  glfwGetCursorPos(window_, &xpos, &ypos);
  xpos += move_x * CURSOR_SPEED * (state.buttons[pads.left_bumper] == GLFW_RELEASE);
  ypos -= move_y * CURSOR_SPEED * (state.buttons[pads.left_bumper] == GLFW_RELEASE);
  glfwSetCursorPos(window_, xpos, ypos);
}

void key_move_comp::process_button_input(GLFWgamepadstate& state, float dt)
{
  // TODO : impl as lambda
  static bool is_pressed = false;
  // Display* display;
  // click
  if (!is_pressed && state.buttons[pads.button_a] == GLFW_PRESS) {
    // display = XOpenDisplay(NULL);
    // XTestFakeButtonEvent(display, 1, True, CurrentTime);
    // XFlush(display);
    // XCloseDisplay(display);
    is_pressed = true;
  }
    // drag -> nothing to do
  else if (is_pressed && state.buttons[pads.button_a] == GLFW_RELEASE) {
    // display = XOpenDisplay(NULL);
    // XTestFakeButtonEvent(display, 1, False, CurrentTime);
    // XFlush(display);
    // XCloseDisplay(display);
    is_pressed = false;
  }
}

void key_move_comp::adjust_axis_errors()
{
  GLFWgamepadstate state;
  glfwGetGamepadState(pad_number_, &state);
  right_error_ = state.axes[pads.move_x];
  up_error_ = state.axes[pads.move_y];
}


void key_move_comp::set_default_mapping()
{
  // axis
  auto leftXfunc = [](float val){return vec3(0.f, 0.f, 0.f); };
}

} // namespace hnll::game