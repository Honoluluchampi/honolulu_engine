struct fdtd_wg_push {
  // dimension of tube
  float h_width;
  // dimension of window
  float w_height;
  float w_width;
  // right edge coordinates of each section
  vec4 edge_x; // accumulative sum of each segment
  vec4 seg_len; // length of each segment
  ivec4 idx;
};

struct fdtd12_push {
  float horn_x_max;
  float horn_y_max;
  int pml_count;
  int whole_x;
  float dx;
  float dt;
  vec2 window_size;
};