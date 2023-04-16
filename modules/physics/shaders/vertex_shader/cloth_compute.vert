#version 450

#extension GL_GOOGLE_include_directive : require

#include "../../../graphics/shaders/global_ubo.h"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

void main()
{
  vec4 pos_world = vec4(in_position, 1.0);
  gl_Position = ubo.projection * ubo.view * pos_world;

  frag_normal_world = in_normal;
  frag_pos_world = pos_world.xyz;
  frag_color = vec3(0.8, 0.8, 0.8);
}