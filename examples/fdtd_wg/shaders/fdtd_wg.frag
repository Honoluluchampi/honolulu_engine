#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push { fdtd_wg_push push; };

layout(set = 0, binding = 0) readonly buffer Field { float field[]; };

const float line_width = 3.f;
const float magnification = 650.f;
const float color_scale = 0.01f;

void main() {
  bool in_length  = abs(gl_FragCoord.x - push.w_width / 2.f) < push.edge_x.z * magnification / 2.f;
  bool out_length = abs(gl_FragCoord.x - push.w_width / 2.f) > push.edge_x.z * magnification / 2.f + line_width;
  bool in_width  = abs(gl_FragCoord.y - push.w_height / 2.f) < push.h_width / 2.f;
  bool out_width = abs(gl_FragCoord.y - push.w_height / 2.f) > push.h_width / 2.f + line_width;

  bool in_area = in_length && in_width;
  bool out_area = out_length || out_width;
  bool on_outline = !in_area && !out_area;

  // outline of field
  if (on_outline)
    out_color = vec4(1.f, 1.f, 1.f, 1.f);

  // draw field
  else if (in_area) {
    float x_coord = push.edge_x.z / 2.f + (gl_FragCoord.x - push.w_width / 2.f) / magnification;
    int idx;
    if (0 <= x_coord && x_coord < push.edge_x.x) {
      idx = int(float(push.idx.x) * (x_coord / push.seg_len.x));
    }
    else if (push.edge_x.x <= x_coord && x_coord < push.edge_x.y) {
      idx = push.idx.x +
        int(float(push.idx.y) * (x_coord - push.edge_x.x) / push.seg_len.y);
    }
    else if (push.edge_x.y <= x_coord && x_coord < push.edge_x.z) {
      idx = push.idx.x + push.idx.y +
        int(float(push.idx.z) * (x_coord - push.edge_x.y) / push.seg_len.z);
    }
    else {
      discard;
    }
    out_color = vec4(color_scale * max(field[idx], 0.f), 0.f, color_scale * max(-field[idx], 0.f), 1.f);
  }

  // out area
  else
    out_color = vec4(0.f, 0.f, 0.f, 1.f);
}