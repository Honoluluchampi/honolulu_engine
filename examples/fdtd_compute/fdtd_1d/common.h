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
  int   open_hole_id;
  float mouth_pressure;
  float debug;
};

struct particle
{
  float p;
  float vx;
  float vy;
  float dummy;
};

struct field_element
{
  float pml;
  float y_offset;
};

const int fdtd1_local_size_x = 32;
const int fdtd1_local_size_y = 1;
const int fdtd1_local_size_z = 1;

const int PV_DESC_SET_ID = 0;
const int FIELD_DESC_SET_ID = 1;
const int SOUND_DESC_SET_ID = 2;
const int PML_COUNT = 6;