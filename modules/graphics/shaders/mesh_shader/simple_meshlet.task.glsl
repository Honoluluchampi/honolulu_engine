#version 460

#extension GL_NV_mesh_shader          : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_GOOGLE_include_directive : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#include "meshlet_constants.h"

// -------------------------------------------------------

layout(local_size_x = GROUP_SIZE) in;

taskNV out task {
    uint base_id;
    uint8_t sub_ids[MESHLET_PER_TASK];
} OUT;

// bindings
// -------------------------------------------------------
// global ubo
#include "../global_ubo.h"

// ------------------------------------------------------
// frustum info

// top, bottom, right and left have same position (camera position)
struct frustum_info {
  vec3 camera_position;
  vec3 near_position;
  vec3 far_position;
  vec3 top_n;
  vec3 bottom_n;
  vec3 right_n;
  vec3 left_n;
  vec3 near_n;
  vec3 far_n;
};

layout(set = 1, binding = 0) uniform _frustum_info {
  frustum_info frustum;
};

// ------------------------------------------------------
// meshlet info

struct meshlet {
  uint vertex_indices   [MAX_VERTEX_COUNT];
  uint primitive_indices[MAX_PRIMITIVE_INDICES_COUNT];
  uint vertex_count; // < MAX_VERTEX_COUNT
  uint index_count; // < MAX_PRIMITIVE_INDICES_COUNT
  // for frustum culling
  vec3 center;
  float radius;
};

layout(set = 3, binding = 0) buffer _mesh_buffer {
  meshlet meshlets[];
};

// ------------------------------------------------------

layout(push_constant) uniform Push {
  mat4 model_matrix;
  mat4 normal_matrix;
} push;

// ------------------------------------------------------

// signed distance
float distance_point_to_plane(vec3 point, vec3 plane_position, vec3 plane_normal) {
  return dot(plane_normal, point - plane_position);
}

// returns true if an object is (partly) included by the frustum
bool sphere_frustum_intersection(vec3 world_center, float radius) {
  bool top    = distance_point_to_plane(world_center, frustum.camera_position, frustum.top_n)    > -radius;
  bool bottom = distance_point_to_plane(world_center, frustum.camera_position, frustum.bottom_n) > -radius;
  bool right  = distance_point_to_plane(world_center, frustum.camera_position, frustum.right_n)  > -radius;
  bool left   = distance_point_to_plane(world_center, frustum.camera_position, frustum.left_n)   > -radius;
  bool near   = distance_point_to_plane(world_center, frustum.near_position,   frustum.near_n)   > -radius;
  bool far    = distance_point_to_plane(world_center, frustum.far_position,    frustum.far_n)    > -radius;
  return top && bottom && right && left && near && far; 
}

uint base_id = gl_WorkGroupID.x * MESHLET_PER_TASK;
uint lane_id = gl_LocalInvocationID.x;
uint task_loop = (MESHLET_PER_TASK + GROUP_SIZE - 1) / GROUP_SIZE;

void main() {
    uint out_meshlet_count = 0;

    for (uint i = 0; i < task_loop; i++) {
      // identify current meshlet
      uint meshlet_local = lane_id + i * GROUP_SIZE;
      uint current_meshlet_index = base_id + meshlet_local;
      meshlet current_meshlet = meshlets[current_meshlet_index];

      vec4 world_center = push.model_matrix * vec4(current_meshlet.center, 1.0);

      // frustum culling 
      bool render = sphere_frustum_intersection(world_center.xyz, current_meshlet.radius);

      // culling is executed in each invocation and those results are gathred
      uvec4 culling_results = subgroupBallot(render);
      uint idxOffset  = subgroupBallotExclusiveBitCount(culling_results) + out_meshlet_count;

      if (render) {
        OUT.sub_ids[idxOffset] = uint8_t(meshlet_local);
      }
      
      out_meshlet_count += subgroupBallotBitCount(culling_results);
    }

    if (lane_id == 0) {
        gl_TaskCountNV = out_meshlet_count;
        OUT.base_id    = base_id;
    }
}