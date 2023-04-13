#version 450

#extension GL_GOOGLE_include_directive : require

#include "../../../graphics/shaders/global_ubo.h"

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

void main()
{
  vec4 pos_world = vec4(in_position, 1.0);
  gl_Position = ubo.projection * ubo.view * pos_world;

  frag_normal_world = vec3(0.0, 0.0, -1.0);
  frag_pos_world = pos_world.xyz;
  frag_color = -in_position;
}