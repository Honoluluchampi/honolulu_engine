#version 450

#extension GL_GOOGLE_include_directive : require

// build in type
// corners of triangles

// in keyword indicates that 'position' gets the data from a vertex buffer
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

// global ubo
#include "../global_ubo.h"

// compatible with a renderer system
layout(push_constant) uniform Push {
  mat4 modelMatrix; // projection * view * model
  mat4 normalMatrix;
} push;

// executed for each vertex
void main()
{
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
  gl_Position = ubo.projection * ubo.view * positionWorld;

  fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
  fragPosWorld = positionWorld.xyz;
  fragColor = color;
}