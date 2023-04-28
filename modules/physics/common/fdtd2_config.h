// be careful not to include this file more than once

const int fdtd2_local_size_x = 32;
const int fdtd2_local_size_y = 32;

struct fdtd2_push {
  // grid count
  int x_grid;
  int y_grid;
  // spatial length
  float x_len;
  float y_len;
  // physical
  float p_fac;
  float v_fac;
  int reset;
};