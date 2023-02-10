#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec3 vertex_position;

// global ubo
#include "../global_ubo.h"

vec3 grid_plane[6] = vec3 [](
  vec3( 1, 0,  1),
  vec3(-1, 0, -1),
  vec3(-1, 0,  1),
  vec3(-1, 0, -1),
  vec3( 1, 0,  1),
  vec3( 1, 0, -1)
);

const float scale = 500.0;

void main() {
  vec3 position = scale * grid_plane[gl_VertexIndex].xyz;
  position.y = 0;
  vertex_position = position;
  gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
}