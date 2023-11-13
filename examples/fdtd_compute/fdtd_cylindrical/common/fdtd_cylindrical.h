// be careful not to include this file more than once
const int fdtd_cylindrical_local_size_x = 16;
const int fdtd_cylindrical_local_size_y = 16;
const int fdtd_cylindrical_local_size_z = 1;

const float DR = 0.1;
const float DZ = 0.1;

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
  // input
  int listener_index;
  // pressure
  float input_pressure;
  int buffer_index;
};

struct particle {
  float p; // pressure
  float vz; // z element of velocity on the cylindrical coordinate
  float vr; // r element
  float state; // pml and grid state
};