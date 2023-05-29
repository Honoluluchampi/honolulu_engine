#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;

struct vertex
{
  vec3 position;
  vec3 normal;
  vec4 color;
};

layout(binding = 0) uniform accelerationStructureEXT tlas;
layout(binding = 2) buffer Vertices { vertex v[]; } vertices;
layout(binding = 3) buffer Indices { uint i[]; } indices;

void main() {
  vertex v0 = vertices.v[indices.i[3 * gl_PrimitiveID];
  vertex v1 = vertices.v[indices.i[3 * gl_PrimitiveID + 1];
  vertex v2 = vertices.v[indices.i[3 * gl_PrimitiveID + 2];

  // normal interpolation
  const vec3 barys = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 normal = normalize(v0.normal * barys.x + v1.normal * barys.y + v2.normal * barys.z);

  hit_value = barys;
}