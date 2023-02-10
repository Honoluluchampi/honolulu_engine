#version 450

#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

// global ubo
#include "../global_ubo.h"

layout(push_constant) uniform Push
{
  vec4 position;
  vec4 color;
  float radius;
} push;

const float M_PI = 3.1415926538;

void main()
{
  float dis = sqrt(dot(fragOffset, fragOffset));
  if (dis >= 1.0) discard; // throw away this fragment
  outColor = vec4(push.color.xyz, 0.5 * (cos(dis * M_PI) + 1.0));
}