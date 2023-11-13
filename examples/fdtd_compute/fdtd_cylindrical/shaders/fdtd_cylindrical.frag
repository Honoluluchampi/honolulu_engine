#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common/fdtd_cylindrical.h"

layout (location = 0) out vec4 out_color;

struct fdtd_cylindrical_frag_push {
  float width;
  float radius;
  ivec2 window_size;
  // grid count
  int z_grid_count;
  int r_grid_count;
};

layout (push_constant) uniform Push { fdtd_cylindrical_frag_push push; };
layout(std430, set = 0, binding = 0) readonly buffer Current { particle curr[]; };

float mergin = 0.05f;

uint g_id(int i, int j) { return i + j * (push.z_grid_count); }

void main()
{
  float x_fix_ratio = push.z_grid_count / ((1 - mergin * 2.f) * push.width);
  float y_fix_ratio = push.r_grid_count / ((1 - mergin * 2.f) * push.radius);
  float fix_ratio = max(x_fix_ratio, y_fix_ratio);
  float x_mergin = (fix_ratio * push.width - push.z_grid_count) / (2 * fix_ratio);
  float y_mergin = (fix_ratio * push.radius - push.r_grid_count) / (2 * fix_ratio);

  float i = (gl_FragCoord.x - x_mergin) * fix_ratio;
  float j = abs((gl_FragCoord.y - push.window_size.y / 2 - y_mergin) * fix_ratio);
  uint idx = g_id(int(i), int(j));

  bool out_of_area = i < 0 || i >= push.z_grid_count || j < 0 || j >= push.r_grid_count;
  float p_val = curr[idx].p;
  p_val *= int(!out_of_area) / 400.f;
  out_color = vec4(p_val, 0, -p_val, 1);

  float state = curr[idx].state;
  // wall
  if (state == -2)
    out_color = vec4(1.f, 1.f, 1.f, 1.f);
  // exciter
  if (state == -1 || state == -3)
    out_color = vec4(0.f, 1.f, 0.f, 1.f);
}