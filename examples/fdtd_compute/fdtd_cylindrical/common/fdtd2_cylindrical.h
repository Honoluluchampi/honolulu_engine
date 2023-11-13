// be careful not to include this file more than once
const int fdtd_cylindrical_local_size_x = 64;
const int fdtd_cylindrical_local_size_y = 1;
const int fdtd_cylindrical_local_size_z = 1;

struct fdtd_cylindrical_push {
  // grid count
  int z_grid;
  int r_grid;
  // domain's dimensions
  float z_len;
  float max_radius;
  // physical constant
  float p_fac;
  float v_fac;
  // input
  int active_grid_count;
  int listener_index;
  // pressure
  float input_pressure;
  int buffer_index;
  // tone hole
  int hole_open;
  float dyn_b;
};