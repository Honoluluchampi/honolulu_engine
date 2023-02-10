#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv0;
layout(location = 3) in vec2 in_uv1;
layout(location = 4) in vec4 in_color;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

// global ubo
#include "../global_ubo.h"

layout(push_constant) uniform Push 
{
  mat4 model_matrix;
  mat4 normal_matrix;
} push;

void main()
{
  vec4 position_world = push.model_matrix * vec4(in_position, 1.0);
  gl_Position = ubo.projection * ubo.view * position_world;

  frag_normal_world = normalize(mat3(push.normal_matrix) * in_normal);
  frag_pos_world = position_world.xyz;
  frag_color = in_color.xyz;
}