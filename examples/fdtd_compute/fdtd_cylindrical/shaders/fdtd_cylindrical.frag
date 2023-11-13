#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common/fdtd_cylindrical_config.h"

layout (location = 0) out vec4 out_color;

struct fdtd_cylindrical_frag_push {
  float width;
  float height;
  // grid count
  int z_grid;
  int r_grid;
};

layout (push_constant) uniform Push { fdtd2_frag_push push; };
layout(std430, set = 0, binding = 0) readonly buffer CurrentP { particle curr_p[]; };

float mergin = 0.05f;

uint g_id(int x, int y) { return (x + 1) + (y + 1) * (push.x_grid + 1); }

void main()
{
  float x_fix_ratio = push.x_grid / ((1 - mergin * 2.f) * push.width);
  float y_fix_ratio = push.y_grid / ((1 - mergin * 2.f) * push.height);
  float fix_ratio = max(x_fix_ratio, y_fix_ratio);
  float x_mergin = (fix_ratio * push.width - push.x_grid) / (2 * fix_ratio);
  float y_mergin = (fix_ratio * push.height - push.y_grid) / (2 * fix_ratio);

  float i = (gl_FragCoord.x - x_mergin) * fix_ratio;
  float j = (gl_FragCoord.y - y_mergin) * fix_ratio;
  uint idx = g_id(int(i), int(j));

  bool out_of_area = i < 0 || i >= push.x_grid || j < 0 || j >= push.y_grid;
  float p_val = curr_p[idx].values.z;
  p_val *= int(!out_of_area) / 400.f;
  out_color = vec4(p_val, 0, -p_val, 1);

  float state = curr_p[idx].values.w;
  // wall
  if (state == -2)
    out_color = vec4(1.f, 1.f, 1.f, 1.f);
  // exciter
  if (state == -1 || state == -3)
    out_color = vec4(0.f, 1.f, 0.f, 1.f);
}