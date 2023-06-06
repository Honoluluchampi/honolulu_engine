#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;

struct vertex
{
  vec3 position;
  vec3 color;
  vec3 normal;
  vec2 uv;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 2) buffer Vertices { vertex v[]; } vertices;
layout(set = 0, binding = 3) buffer Indices { uint i[]; } indices;

vec3 light_direction = normalize(vec3(-1.0, -1.0, 1.0));

void main() {
  vertex v0 = vertices.v[indices.i[3 * gl_PrimitiveID]];
  vertex v1 = vertices.v[indices.i[3 * gl_PrimitiveID + 1]];
  vertex v2 = vertices.v[indices.i[3 * gl_PrimitiveID + 2]];

  // normal interpolation
  const vec3 barys = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 normal = normalize(v0.normal * barys.x + v1.normal * barys.y + v2.normal * barys.z);
  vec3 color  = normalize(v0.color.rgb * barys.x + v1.color.rgb * barys.y + v2.color.rgb * barys.z);

  float dot_nl = max(dot(light_direction, normal), 0.1);

  hit_value = color * dot_nl;
}