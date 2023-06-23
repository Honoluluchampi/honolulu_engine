struct fdtd_wg_push {
  // dimension of tube
  float h_width;
  // dimension of window
  float w_height;
  float w_width;
  // right edge coordinates of each section
  vec3 edge_x; // accumulative sum of each segment
  vec3 seg_len; // length of each segment
  ivec3 idx;
};