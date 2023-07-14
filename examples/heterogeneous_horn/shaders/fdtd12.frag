#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

// x : x velo, y : y velo, z : pressure
layout(set = 0, binding = 0) readonly buffer Field { vec4 field[]; };
layout(set = 1, binding = 0) readonly buffer GridCond { vec4 grid_conditions[]; };
// x, y : size of segment, w : dimension
layout(set = 2, binding = 0) readonly buffer SegInfo { vec4 segment_infos[]; };
// x : x edge, y : starting_grid_id
layout(set = 3, binding = 0) readonly buffer EdgeInfo { vec4 edge_infos[]; };
// y = 1 if 1D
layout(set = 4, binding = 0) readonly buffer GridCount { ivec2 grid_counts[]; };

const float line_width = 3.f;
const float magnification = 650.f;
const float x_mergin_ratio = 0.1f;
const float color_scale = 0.007f;

layout(push_constant) uniform Push { fdtd12_push push; };

vec4[10] debug_color = vec4[](
  vec4(0.f, 0.f, 0.f, 1.f),   // EMPTY
  vec4(0.5f, 0.f, 0.f, 1.f),  // NORMAL1
  vec4(0.f, 0.f, 0.5f, 1.f),  // NORMAL2
  vec4(1.f, 1.f, 1.f, 1.f),   // WALL
  vec4(0.f, 1.f, 0.1f, 1.f),  // EXCITER
  vec4(0.f, 0.f, 1.f, 1.f),   // PML
  vec4(0.7f, 0.7f, 0.f, 1.f), // JUNCTION_12_LEFT
  vec4(0.7f, 0.f, 0.7f, 1.f), // JUNCTION_12_RIGHT
  vec4(0.f, 0.7f, 0.7f, 1.f), // JUNCTION_21_LEFT
  vec4(0.2f, 0.2f, 0.7f, 1.f) // JUNCTION_21_RIGHT
);

void main() {
  float fix_ratio = push.horn_x_max / ((1 - x_mergin_ratio * 2.f) * push.window_size.x);
  float x_coord = (gl_FragCoord.x - x_mergin_ratio * push.window_size.x) * fix_ratio;
  float y_coord = -(gl_FragCoord.y - push.window_size.y / 2.f) * fix_ratio;

  out_color = vec4(0.f, 0.f, 0.f, 1.f);

  if (x_coord < 0.f || x_coord > push.horn_x_max) {
    return;
  }

  int x_idx = int(x_coord / push.dx);
  int y_idx = int(y_coord / push.dx);
  int idx = x_idx + y_idx * push.whole_x;

  int state = int(grid_conditions[idx].x);
  out_color = debug_color[state];

  return;
}