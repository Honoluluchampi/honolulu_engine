#version 450

#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 frustum_color;
layout (location = 1) in vec2 uv;
layout (location = 0) out vec4 out_color;

// global ubo
#include "../global_ubo.h"

layout(push_constant) uniform Push {
  mat4 modelMatrix;
} push;

const float edge_width = 1;

float edge_factor() {
  vec2 d = fwidth(uv);
  vec2 f = step( d * edge_width, uv);
  return min(f.x, f.y);
}

void main()
{  
  if (edge_factor() > 0.4)
    discard;
  out_color = vec4(frustum_color, 1.0);
}