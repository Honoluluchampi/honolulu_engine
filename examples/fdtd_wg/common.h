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

struct fdtd_2d_push {
  // dimension of tube
  vec2 h_dim;
  // dimension of window
  vec2 w_dim;
  ivec2 grid_count; // grid count of each axis
};