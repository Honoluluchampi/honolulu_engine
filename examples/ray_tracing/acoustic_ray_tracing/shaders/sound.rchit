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
layout(set = 0, binding = 1) buffer Vertices { vertex v[]; } vertices;
layout(set = 0, binding = 2) buffer Indices { uint i[]; } indices;

vec3 sphere1_center = vec3(-6, -4, 0);
vec3 sphere2_center = vec3(2.5, -1.5, 4);
float sphere_radius = 0.5f;

const int MAX_HIT = 5;

void main() {
  vertex v0 = vertices.v[indices.i[3 * gl_PrimitiveID]];
  vertex v1 = vertices.v[indices.i[3 * gl_PrimitiveID + 1]];
  vertex v2 = vertices.v[indices.i[3 * gl_PrimitiveID + 2]];

  // normal interpolation
  const vec3 barys = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 normal = normalize(v0.normal * barys.x + v1.normal * barys.y + v2.normal * barys.z);

  hit_value.z += gl_HitTEXT;

  // if ray hits sound source
  bool hit_flag = false;
  vec3 hit_point = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  if (distance(hit_point, sphere1_center) <= sphere_radius + 0.01) {
    hit_value.y = 1;
    hit_flag = true;
  }
  if (distance(hit_point, sphere2_center) <= sphere_radius + 0.01) {
    hit_value.y = 2;
    hit_flag = true;
  }

  // continue to reflect ray
  if (!hit_flag && hit_value.x < MAX_HIT) {
    hit_value.x += 1;
    float tmin = 0.001;
    float tmax = 10000.0;
    // current hit point
    vec3 ray_d = normalize(gl_WorldRayDirectionEXT);
    vec3 direction = normalize(ray_d - 2 * dot(ray_d, normal) * normal);

    traceRayEXT(
      tlas,
      gl_RayFlagsOpaqueEXT,
      0xff,
      0,
      0,
      0,
      hit_point,
      tmin,
      direction,
      tmax,
      0
    );
  }
}