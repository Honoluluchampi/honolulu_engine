#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform Push { fdtd_push push; };

float max_length = 0.6f; // meter
float mergin = 0.05f;
float dx_ = 0.001;

void main()
{
  float len_to_grid = push.window_size.x * (1.f - 2.f * mergin) / max_length;
  uint x_offset = uint(push.window_size.x * mergin + (max_length - push.len) / 2.f * len_to_grid);
  bool inside_area = x_offset <= gl_FragCoord.x && gl_FragCoord.x < push.window_size.x - x_offset;
  out_color = vec4(0.f, 0.f, 1.f * float(inside_area), 1.f);
}