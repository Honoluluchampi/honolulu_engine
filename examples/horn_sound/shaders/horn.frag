#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push { horn_push push; };

const float line_width = 3.f;

void main() {
  bool on_length = abs(gl_FragCoord.x - push.w_width / 2.f) < push.h_length / 2.f;
  bool on_width  = abs(gl_FragCoord.y - push.w_height / 2.f) < push.h_width / 2.f + line_width
                && abs(gl_FragCoord.y - push.w_height / 2.f) > push.h_width /2.f;

  float val = float(on_length) * float(on_width) * 1.f;
  out_color = vec4(vec3(val, val, val), 1.f);
}