#version 450

#extension GL_GOOGLE_include_directive : require

#include "../../../graphics/shaders/global_ubo.h"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

struct vertex {
 vec3 position;
 vec3 normal;
};

vec3 positions[3] = {
  vec3(-1.f, 0.f, 0.f),
  vec3(1.f, 0.f, 0.f),
  vec3(0.f, 0.f, 1.f)
};

void main()
{
  // vec4 pos_world = vec4(in_position, 1.0);
  vec4 pos_world = vec4(positions[gl_VertexIndex % 3], 1.0);
  gl_Position = ubo.projection * ubo.view * pos_world;

  frag_normal_world = vec3(0.0, 0.0, -1.0);
  frag_pos_world = pos_world.xyz;
  frag_color = vec3(0.0, 0.4, 0.8);
}