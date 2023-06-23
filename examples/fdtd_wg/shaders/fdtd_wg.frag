#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push { fdtd_wg_push push; };

layout(set = 0, binding = 0) readonly buffer Field { float field[]; };

const float line_width = 3.f;
const float magnification = 500.f;

void main() {
  bool on_length = abs(gl_FragCoord.x - push.w_width / 2.f) < push.edge_x.z / 2.f;
  bool on_width  = abs(gl_FragCoord.y - push.w_height / 2.f) < push.h_width / 2.f + line_width
  && abs(gl_FragCoord.y - push.w_height / 2.f) > push.h_width /2.f;

  // outline of field
  float val = float(on_length) * float(on_width) * 1.f;
  if (on_length && on_width) {
    out_color = vec4(val, val, val, 1.f);
    return;
  }

  // out of the field
  bool in_width = abs(gl_FragCoord.y - push.w_height / 2.f) < push.h_width /2.f;
  if (!on_length || !in_width) {
    discard;
  }

  float x_coord = push.edge_x.z / 2.f + (gl_FragCoord.x - push.w_width / 2.f) / magnification;
  int idx;
  if (0 <= x_coord && x_coord < push.edge_x.x) {
    idx = int(float(push.idx.x) * (x_coord / push.seg_len.x));
  }
  else if (push.edge_x.x <= x_coord && x_coord < push.edge_x.y) {
    idx = push.idx.x +
      int(float(push.idx.y - push.idx.x) * (x_coord - push.edge_x.x) / push.seg_len.y);
  }
  else if (push.edge_x.y <= x_coord && x_coord < push.edge_x.z) {
    idx = push.idx.y +
      int(float(push.idx.z - push.idx.y) * (x_coord - push.edge_x.y) / push.seg_len.z);
  }
  else {
    discard;
  }
  out_color = vec4(max(field[idx], 0.f), 0, max(-field[idx], 0.f), 0.f);
}