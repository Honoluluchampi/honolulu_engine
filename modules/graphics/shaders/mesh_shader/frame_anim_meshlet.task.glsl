#version 460

#extension GL_NV_mesh_shader          : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_GOOGLE_include_directive : require

#include "meshlet_constants.h"

uint base_id = gl_WorkGroupID.x * MESHLET_PER_TASK;
uint lane_id = gl_LocalInvocationID.x;

// -------------------------------------------------------

layout(local_size_x = MESHLET_PER_TASK) in;

taskNV out task {
    uint base_id;
    uint8_t sub_ids[MESHLET_PER_TASK];
} OUT;

// bindings
// ------------------------------------------------------------------------
// global ubo
#include "../global_ubo.h"

// ------------------------------------------------------
// frustum info

// top, bottom, right, left have same position (camera position)
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

// ------------------------------------------------------------------------
// meshlet buffer
// one storage buffer binding can have only one flexible length array

struct meshlet {
  uint vertex_indices   [MAX_VERTEX_COUNT];
  uint primitive_indices[MAX_PRIMITIVE_INDICES_COUNT];
  uint vertex_count; // < MAX_VERTEX_COUNT
  uint index_count; // < MAX_PRIMITIVE_INDICES_COUNT
};

struct sphere {
  vec4 center_and_radius; // .xyz : center position, .w : radius
};

struct common_attribute {
  vec2 uv0;
  vec2 uv1;
  vec4 color;
};

struct dynamic_attribute {
  vec3 position;
  vec3 normal;
};

layout(set = 2, binding = 0) buffer CommonAttribs {
  common_attribute common_attributes[];
};

layout(set = 3, binding = 0) buffer MeshBuffer {
  meshlet meshlets[];
};

layout(set = 4, binding = 0) buffer DynamicAttribs {
  dynamic_attribute dynamic_attributes[];
};

layout(set = 5, binding = 0) buffer BoundingSphere {
  sphere bounding_spheres[];
};

// ------------------------------------------------------------------------

layout(push_constant) uniform Push {
  mat4 model_matrix;
  mat4 normal_matrix;
} push;

#define COLOR_COUNT 10
vec3 meshlet_colors[COLOR_COUNT] = {
  vec3(1,0,0),
  vec3(0,1,0),
  vec3(0,0,1),
  vec3(1,1,0),
  vec3(1,0,1),
  vec3(0,1,1),
  vec3(1,0.5,0),
  vec3(0.5,1,0),
  vec3(0,0.5,1),
  vec3(1,1,1)
};

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

void main() {
    uint out_meshlet_count = 0;

    for (uint i = 0; i < MESHLET_PER_TASK; i++) {
      uint meshlet_local = lane_id + i;
      uint current_meshlet_index = base_id + meshlet_local;
      meshlet current_meshlet = meshlets[current_meshlet_index];
      vec4  current_sphere = bounding_spheres[current_meshlet_index].center_and_radius;

     vec4 world_center = push.model_matrix * vec4(current_sphere.xyz, 1.0);

      if (sphere_frustum_intersection(world_center.xyz, current_sphere.w)) {
        OUT.sub_ids[out_meshlet_count] = uint8_t(meshlet_local);
        out_meshlet_count += 1;
      }
    }

    if (lane_id == 0) {
        gl_TaskCountNV = out_meshlet_count;
        OUT.base_id    = base_id;
    }
}