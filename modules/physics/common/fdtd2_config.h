// be careful not to include this file more than once

const int fdtd2_local_size_x = 32;
const int fdtd2_local_size_y = 32;

struct fdtd2_push {
  // grid count
  int x_grid;
  int y_grid;
  // domain's dimensions
  float x_len;
  float y_len;
  // physical constant
  float p_fac;
  float v_fac;
  // input
  int buffer_index;
  int listener_index;
  // pressure
  float input_pressure;
  float dummy;
};