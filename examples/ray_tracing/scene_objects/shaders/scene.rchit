#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;
layout(binding = 2, set = 0) uniform scene_parameters {
    mat4 view;
    mat4 projection;
    vec4 light_direction;
    vec4 light_color;
    vec4 ambient_color;
} ubo;

struct vertex { vec3 position; vec3 normal; vec4 color; };

// vertex and index buffer
layout(buffer_reference, scalar) buffer vertex_buffer {
  vertex v[];
};
layout(buffer_reference, scalar) buffer index_buffer {
  uvec3 i[];
};
layout(shaderRecordEXT, std430) buffer shader_binding_table_data {
  index_buffer  indices;
  vertex_buffer vertices;
};

// hit triangle is notified by barycentric coodinate values
vertex calc_vertex(vec3 barys) {
  const uvec3 id = indices.i[gl_PrimitiveID];
  vertex v0 = vertices.v[id.x];
  vertex v1 = vertices.v[id.y];
  vertex v2 = vertices.v[id.z];

  vertex ret;
  ret.position = v0.position * barys.x + v1.position * barys.y + v2.position * barys.z;
  ret.normal   = v0.normal * barys.x + v1.normal * barys.y + v2.normal * barys.z;
  ret.color    = v0.color * barys.x + v1.color * barys.y + v2.color * barys.z;
  return ret;
}

void main() {
  vec3 barys = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  vertex vert = calc_vertex(barys);

  // translate into world position (vertex buffer's data is local coord)
  vec3 world_normal = mat3(gl_ObjectToWorldEXT) * vert.normal;

  // lighting
  const vec3 to_light = normalize(-ubo.light_direction.xyz);
  float dot_n_l = max(dot(world_normal, to_light), 0.0);


  hit_value = vert.color.xyz * dot_n_l * ubo.light_color.xyz;
  hit_value += vert.color.xyz * ubo.ambient_color.xyz;
}