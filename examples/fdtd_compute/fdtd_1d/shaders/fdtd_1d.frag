#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform Push { fdtd_push push; };
layout (std430, set = 0, binding = 0) readonly buffer Yoffsets { float y_offsets[]; };

float max_length = 0.6f; // meter
float mergin = 0.05f;
float edge_width = 0.004f;

void main()
{
  float len_to_pixel = push.window_size.x * (1.f - 2.f * mergin) / max_length;
  uint x_offset = uint(push.window_size.x * mergin + (max_length - push.len) / 2.f * len_to_pixel);

  int x_id = int((gl_FragCoord.x - x_offset) / len_to_pixel / push.dx);
  float y_offset = y_offsets[x_id];

  bool x_inside_area = (0 < x_id) && (x_id < push.grid_count);
  bool y_on_edge = abs(gl_FragCoord.y - push.window_size.y / 2.f) / len_to_pixel <= y_offset + edge_width
    && abs(gl_FragCoord.y - push.window_size.y / 2.f) / len_to_pixel > y_offset;
  bool inside_area = x_inside_area && y_on_edge;

  out_color = vec4(vec3(1.f, 1.f, 1.f) * float(inside_area), 1.f);
}