#version 450

#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout (location = 0) out vec4 out_color;

// x : x velo, y : y velo, z : pressure
layout(set = 0, binding = 0) readonly buffer Field { vec4 field[]; };
layout(set = 1, binding = 0) readonly buffer GridCond { vec4 grid_conditions[]; };
// x, y : size of segment, w : dimension
layout(set = 2, binding = 0) readonly buffer SegInfo { vec4 segment_infos[]; };
// x : x edge, y : starting_grid_id
layout(set = 3, binding = 0) readonly buffer EdgeInfo { vec4 edge_infos[]; };
// y = 1 if 1D
layout(set = 4, binding = 0) readonly buffer GridCount { ivec2 grid_counts[]; };

const float line_width = 3.f;
const float magnification = 650.f;
const float x_mergin_ratio = 0.1f;
const float color_scale = 0.007f;

struct fdtd12_push {
  int segment_count; // element count of segment info
  float horn_x_max;
  int pml_count;
  int whole_grid_count;
  vec2 window_size;
};

layout(push_constant) uniform Push { fdtd12_push push; };

void main() {
  float fix_ratio = push.horn_x_max / ((1 - x_mergin_ratio * 2.f) * push.window_size.x);
  float x_coord = (gl_FragCoord.x - x_mergin_ratio * push.window_size.x) * fix_ratio;
  float y_coord = -(gl_FragCoord.y - push.window_size.y / 2.f) * fix_ratio;

  out_color = vec4(0.f, 0.f, 0.f, 1.f);

  if (x_coord < 0.f || x_coord > push.horn_x_max) {
    return;
  }
  else {
    for (int i = 0; i < push.segment_count; i++) {
      if (x_coord < edge_infos[i + 1].x) {
        if (abs(y_coord) < segment_infos[i].y / 2.f) {
          float local_x = x_coord - edge_infos[i].x;
          float local_y = y_coord + 0.5 * segment_infos[i].y;
          int is_last = int(i == push.segment_count - 1);
          int x_idx = int(local_x / segment_infos[i].x * (grid_counts[i].x - 2 * push.pml_count * is_last))
                      + push.pml_count * is_last;
          int y_idx = int(local_y / segment_infos[i].y * (grid_counts[i].y - 2 * push.pml_count * is_last))
                      + push.pml_count * is_last;
          int idx = int(edge_infos[i].y) + y_idx + int(x_idx * (grid_counts[i].y));
          float val = field[idx].z / push.whole_grid_count;
          int is_normal  = int(grid_conditions[idx].x == 0);
          int is_exciter = int(grid_conditions[idx].x == 1);
          int is_pml     = int(grid_conditions[idx].x == 2);
          int is_junc12  = int(grid_conditions[idx].x == 4);
          int is_junc21  = int(grid_conditions[idx].x == 5);

          float c = val / 256.f;
          out_color = vec4(0, 86.f * c, 56.f * c, 1.f);
          out_color = vec4(is_normal, is_exciter + is_junc21, is_junc12 + is_junc21, 1.f);
        }
        return;
      }
    }
  }
}