#version 460
#extension GL_NV_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_GOOGLE_include_directive : require

#include "meshlet_constants.h"

layout(local_size_x = 32) in;

// identifier "triangles" indicates this shader outputs trianlges (other candidates : point, line)
// gl_MeshVerticesNV and glPrimitiveIndicesNV is resized according to these values
layout(triangles, max_vertices = MAX_VERTEX_COUNT, max_primitives = MAX_PRIMITIVE_INDICES_COUNT / 3) out;

// inputs from task shader
//taskNV in Task {
taskNV in task {
  uint    base_id;
  uint8_t sub_ids[MESHLET_PER_TASK];
} IN;
// // gl_WorkGroupID.x runs from [0 .. parentTask.gl_TaskCountNV - 1]
// uint meshletID = IN.baseID + IN.deltaIDs[gl_WorkGroupID.x];

// pass to fragment shader
layout (location = 0) out PerVertexData {
  vec4 color;
} v_out[];

// bindings
// ------------------------------------------------------------------------
// global ubo
#include "../global_ubo.h"

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
  sphere bounding_sphere[];
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

void main() {

  // following three gl_~NV variables are built in variables for mesh shading
  // detect current meshlet
  uint meshlet_index = IN.base_id + IN.sub_ids[gl_WorkGroupID.x];
  meshlet current_meshlet = meshlets[meshlet_index];

  //------- vertex processing ---------------------------------------------
  const uint vertex_loops = (MAX_VERTEX_COUNT + GROUP_SIZE - 1) / GROUP_SIZE;
  
  uint vertex_count = current_meshlet.vertex_count;

  mat4 pvw_mat = ubo.projection * ubo.view * push.model_matrix;

  for (uint loop = 0; loop < vertex_loops; loop++) {
    // distribute execution across threads
    uint v = gl_LocalInvocationID.x + loop * GROUP_SIZE;
    v = min(v, vertex_count - 1);
    {
      // vertex_index indicates the vertex_buffer's index
      uint vertex_index = current_meshlet.vertex_indices[v];

      gl_MeshVerticesNV[v].gl_Position = pvw_mat * vec4(dynamic_attributes[vertex_index].position, 1.0);
      v_out[v].color = vec4(meshlet_colors[meshlet_index %COLOR_COUNT], 1.f);
    }
  }

  //------- index processing ----------------------------------------------
  uint index_count = current_meshlet.index_count;
  gl_PrimitiveCountNV = uint(index_count) / 3;

  for (uint i = 0; i < index_count; i++) {
    gl_PrimitiveIndicesNV[i] = current_meshlet.primitive_indices[i];
  }
}