// be careful not to include this file more than once

const int fdtd2_local_size_x = 64;
const int fdtd2_local_size_y = 1;
const int fdtd2_local_size_z = 1;

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
  int active_grid_count;
  int listener_index;
  // pressure
  float input_pressure;
  int buffer_index;
};