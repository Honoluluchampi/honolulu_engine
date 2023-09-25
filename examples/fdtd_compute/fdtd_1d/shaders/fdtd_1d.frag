#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform Push { fdtd_push push; };

void main()
{
  out_color = vec4(0.f, 0.f, 1.f, 1.f);
}