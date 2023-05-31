#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hit_value;
layout(location = 1) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

struct vertex
{
  vec3 position;
  vec3 color;
  vec3 normal;
  vec2 uv;
};

layout(binding = 0) uniform accelerationStructureEXT tlas;
layout(binding = 2) buffer Vertices { vertex v[]; } vertices;
layout(binding = 3) buffer Indices { uint i[]; } indices;

vec3 light_direction = normalize(vec3(0.0, 0.0, 1.0));

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

  // check shadow
  float tmin = 0.001;
  float tmax = 10000.0;
  // current hit point
  vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * glHiTEXT;
  shadowed = true;
  traceRayEXT(
    tlas,
    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
    0xFF,
    0,
    0,
    1,
    origin,
    tmin,
    light_direction,
    tmax,
    2);

  if (shadowed) {
    hit_value *= 0.3;
  }
}