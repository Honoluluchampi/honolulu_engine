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

layout(set = 0, binding = 0, rgba32f) readonly uniform image2D curr;

ivec2 g_id(uint x, uint y) { return ivec2(x + 1, y + 1); }

void main()
{
  float i = (gl_FragCoord.x ) / push.width * push.x_grid;
  float j = (1 - gl_FragCoord.y / push.height) * push.y_grid;
  float p_val = imageLoad(curr, g_id(uint(i), uint(j))).z / 128.f;

  out_color = vec4(max(p_val, 0.f), 0, max(-p_val, 0.f), 1);
}