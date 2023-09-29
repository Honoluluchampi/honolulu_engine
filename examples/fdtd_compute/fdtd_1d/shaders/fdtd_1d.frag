#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

// field
layout (std430, set = 1, binding = 0) readonly buffer Curr { particle curr[]; };
layout (std430, set = 2, binding = 0) readonly buffer Field { field_element field[]; };
layout (push_constant) uniform Push { fdtd_push push; };

float max_length = 0.6f; // meter
float mergin = 0.05f;
float edge_width = 0.004f;

vec3 positive_color = vec3(1.f, 0.2f, 0.f);
vec3 negative_color = vec3(0.f, 0.2f, 1.f);

void main()
{
  float len_to_pixel = push.window_size.x * (1.f - 2.f * mergin) / max_length;
  uint x_offset = uint(push.window_size.x * mergin + (max_length - push.len) / 2.f * len_to_pixel);

  int x_id = int((gl_FragCoord.x - x_offset) / len_to_pixel / push.dx);
  float y_offset = field[x_id].y_offset;

  float y_offset_this_pixel = abs(gl_FragCoord.y - push.window_size.y / 2.f) / len_to_pixel;
  bool is_upper = (gl_FragCoord.y - push.window_size.y / 2.f) < 0;

  bool x_inside_area = (0 < x_id) && (x_id < push.main_grid_count);
  bool y_inside_area = y_offset_this_pixel <= y_offset;
  bool y_on_edge = y_offset_this_pixel <= y_offset + edge_width
    && y_offset_this_pixel > y_offset;

  bool inside_area = x_inside_area && y_inside_area;
  bool on_edge = x_inside_area && y_on_edge && ((x_id != 60 && is_upper) || !is_upper);

  float p = curr[x_id].p / 500.f;
  out_color.w = 1.f;
  out_color.xyz = float(on_edge) * vec3(1.f, 1.f, 1.f);
  out_color.xyz += max(p, 0.f) * float(inside_area) * positive_color;
  out_color.xyz += max(-p, 0.f) * float(inside_area) * negative_color;
}