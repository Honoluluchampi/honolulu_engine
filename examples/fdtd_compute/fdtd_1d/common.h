struct fdtd_push
{
  ivec2 window_size;
  float len;
  float dx;
  float phase;
  float v_fac;
  float p_fac;
  int   main_grid_count;
  int   whole_grid_count;
  int   sound_buffer_id;
};

struct particle
{
  float p;
  float v;
  float pml;
  float y_offset;
};

const int fdtd1_local_size_x = 32;
const int fdtd1_local_size_y = 1;
const int fdtd1_local_size_z = 1;