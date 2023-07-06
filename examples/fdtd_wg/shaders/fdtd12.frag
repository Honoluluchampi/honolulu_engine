#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;


layout(set = 0, binding = 0) readonly buffer Field { vec4 field[]; };
layout(set = 1, binding = 0) readonly buffer GridCond { vec4 grid_conditions[]; };
// x, y : size of segment, z : dimension of segment
layout(set = 2, binding = 0) readonly buffer SegInfo { vec4 segment_info[]; };

const float line_width = 3.f;
const float magnification = 650.f;
const float x_mergin_ratio = 0.1f;
const float color_scale = 0.007f;

struct fdtd12_push {
  int segment_count; // element count of segment info
  float horn_x_max;
  vec2 window_size;
};

layout(push_constant) uniform Push { fdtd12_push push; };

void main() {
  float fix_ratio = push.horn_x_max / ((1 - x_mergin_ratio * 2.f) * push.window_size.x);
  float x_coord = (gl_FragCoord.x - x_mergin_ratio * push.window_size.x) * fix_ratio;
  float y_coord = -(gl_FragCoord.y - push.window_size.y / 2.f) * fix_ratio;

  if (x_coord < 0.f || x_coord > push.horn_x_max) {
    out_color = vec4(0.f, 0.f, 0.f, 1.f);
  }
  else
    out_color = vec4(x_coord, y_coord, -y_coord, 1.f);
}