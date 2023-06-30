#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push { fdtd_2d_push push; };

layout(set = 0, binding = 0) readonly buffer Field { float field[]; };

const float line_width = 3.f;
const float magnification = 1200.f;
const float color_scale = 0.1f;

int ELEM_2D(int i, int j) { return i + push.grid_count.x * j; }

void main() {
  bool in_x  = abs(gl_FragCoord.x - push.w_dim.x / 2.f) < push.h_dim.x * magnification / 2.f;
  bool out_x = abs(gl_FragCoord.x - push.w_dim.x / 2.f) > push.h_dim.x * magnification / 2.f + line_width;
  bool in_y  = abs(gl_FragCoord.y - push.w_dim.y / 2.f) < push.h_dim.y * magnification / 2.f;
  bool out_y = abs(gl_FragCoord.y - push.w_dim.y / 2.f) > push.h_dim.y * magnification / 2.f + line_width;

  bool in_area = in_x && in_y;
  bool out_area = out_x || out_y;
  bool on_outline = !in_area && !out_area;

  // outline of field
  if (on_outline)
    out_color = vec4(1.f, 1.f, 1.f, 1.f);

  // draw field
  else if (in_area) {
    // calc the coord of the pixel
    float x_coord = push.h_dim.x / 2.f + (gl_FragCoord.x - push.w_dim.x / 2.f) / magnification;
    float y_corrd = push.h_dim.y / 2.f + (gl_FragCoord.y - push.w_dim.y / 2.f) / magnification;
    int idx_x = int(float(push.grid_count.x) * x_coord / push.h_dim.x);
    int idx_y = int(float(push.grid_count.y) * y_corrd / push.h_dim.y);

    float val = field[ELEM_2D(idx_x, idx_y)];
    out_color = vec4(color_scale * max(val, 0.f), 0.f, color_scale * max(-val, 0.f), 1.f);
  }

  // out area
  else
    out_color = vec4(0.f, 0.f, 0.f, 1.f);
}