#version 450

#extension GL_GOOGLE_include_directive : require

// build in type
// corners of triangles

// in keyword indicates that 'position' gets the data from a vertex buffer
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

// global ubo
#include "../global_ubo.h"

// compatible with a renderer system
layout(push_constant) uniform Push {
  mat4 model_matrix; // projection * view * model
  mat4 normal_matrix;
} push;

// executed for each vertex
void main()
{
  vec4 position_world = push.model_matrix * vec4(position, 1.0);
  gl_Position = ubo.projection * ubo.view * position_world;

  frag_normal_world = normalize(mat3(push.normal_matrix) * normal);
  frag_pos_world = position_world.xyz;
  frag_uv = uv;
}