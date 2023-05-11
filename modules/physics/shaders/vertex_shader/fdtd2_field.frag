#version 450

#extension GL_GOOGLE_include_directive : require

#include "../../common/fdtd2_config.h"
#include "../../common/fdtd_struct.h"

layout (location = 0) out vec4 out_color;

struct fdtd2_frag_push {
  float width;
  float height;
  int x_grid;
  int y_grid;
};

layout (push_constant) uniform Push { fdtd2_frag_push push; };

layout(std430, set = 0, binding = 0) readonly buffer CurrentP { particle curr_p[]; };
//layout(std430, set = 0, binding = 1) readonly buffer CurrentVx { particle curr_vx[]; };
//layout(std430, set = 0, binding = 2) readonly buffer CurrentVy { particle curr_vy[]; };

uint id(int x, int y) { return x + y * push.x_grid; }

void main()
{
  float i = (gl_FragCoord.x ) / push.width * push.x_grid;
  float j = (1 - gl_FragCoord.y / push.height) * push.y_grid;
  float p_val = curr_p[id(int(i), int(j))].values.z / 128.f;
  float x_val = curr_p[id(int(i), int(j))].values.x / 128.f;
  float y_val = curr_p[id(int(i), int(j))].values.y / 128.f;

  out_color = vec4(abs(p_val), abs(x_val), abs(y_val), 1);
}