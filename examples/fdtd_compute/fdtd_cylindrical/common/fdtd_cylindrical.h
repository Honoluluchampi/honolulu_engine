// be careful not to include this file more than once
const int fdtd_cylindrical_local_size_x = 16;
const int fdtd_cylindrical_local_size_y = 16;
const int fdtd_cylindrical_local_size_z = 1;

const float DR = 3.83e-3;
const float DZ = 3.83e-3;
const float DT = 7.81e-6;

const int WALL = -2;
const int EXCITER = -3;

struct fdtd_cylindrical_push {
  // grid count
  int z_grid_count;
  int r_grid_count;
  // domain's dimensions
  float z_len;
  float r_len;
  // physical constant
  float p_fac;
  float v_fac;
  // pressure
  float input_pressure;
  // input
  int listener_index;
  int buffer_index;
  int fcm_source_grid_id;
};

struct fdtd_cylindrical_frag_push {
  int z_pixel_count;
  int r_pixel_count;
  // grid count
  int z_grid_count;
  int r_grid_count;

  int listener_index;
  int dummy;
};

struct particle {
  float p; // pressure
  float vz; // z element of velocity on the cylindrical coordinate
  float vr; // r element
  float state; // pml and grid state
};