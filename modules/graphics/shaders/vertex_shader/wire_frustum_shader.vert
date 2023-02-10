#version 450 

#extension GL_GOOGLE_include_directive : require

// in keyword indicates that 'position' gets the data from a vertex buffer
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 vertex_uv;

layout(location = 0) out vec3 frustum_color;
layout(location = 1) out vec2 uv;

// global ubo
#include "../global_ubo.h"

// compatible with a renderer system
layout(push_constant) uniform Push {
  // more efficient than cpu's projection
  // mat4 projectionMatrix;
  mat4 modelMatrix; // projection * view * model
} push;

// executed for each vertex
void main()
{
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
  gl_Position = ubo.projection * ubo.view * positionWorld;

  frustum_color = color;
  uv = vertex_uv;
}