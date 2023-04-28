#version 450

layout (location = 0) out vec4 out_color;

struct fdtd2_frag_push {
  float width;
  float height;
  int x_grid;
  int y_grid;
};

layout (push_constant) uniform Push { fdtd2_frag_push push; };

layout(std430, set = 0, binding = 0) readonly buffer CurrentP { float curr_p[]; };
layout(std430, set = 0, binding = 1) readonly buffer CurrentVx { float curr_vx[]; };
layout(std430, set = 0, binding = 2) readonly buffer CurrentVy { float curr_vy[]; };

uint id(int x, int y) { return x + y * push.x_grid; }

void main()
{
  float i = (gl_FragCoord.x ) / push.width * push.x_grid;
  float j = (1 - gl_FragCoord.y / push.height) * push.y_grid;
  float val = curr_p[id(int(i), int(j))] / 128.f;

  out_color = vec4(val, 0, -val, 1);
}