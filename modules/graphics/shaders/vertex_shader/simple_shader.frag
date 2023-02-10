#version 450

#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec3 frag_pos_world;
layout (location = 2) in vec3 frag_normal_world;
layout (location = 0) out vec4 out_color;

// global ubo
#include "../global_ubo.h"

layout(push_constant) uniform Push {
  mat4 model_matrix;
  vec3 normal_matrix;
} push;

void main() 
{
  vec3 diffuse_light = ubo.ambient_light_color.xyz * ubo.ambient_light_color.w;
  vec3 specular_light = vec3(0.0);
  vec3 surface_normal = normalize(frag_normal_world);

  vec3 camera_pos_world = ubo.inv_view[3].xyz;
  vec3 view_direction = normalize(camera_pos_world - frag_pos_world);

  for (int i = 0; i < ubo.num_lights; i++) {
    PointLight light = ubo.point_lights[i];
    vec3 direction_to_light = light.position.xyz - frag_pos_world;
    float attenuation = 1.0 / dot(direction_to_light, direction_to_light); // distance squared

    direction_to_light = normalize(direction_to_light);
    float cos_ang_incidence = max(dot(surface_normal, direction_to_light), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;

    diffuse_light += intensity * cos_ang_incidence;
    
    // specular light
    vec3 half_angle = normalize(direction_to_light + view_direction);
    float blinn_term = dot(surface_normal, half_angle);
    blinn_term = clamp(blinn_term, 0 , 1);
    blinn_term = pow(blinn_term, 50);
    specular_light += light.color.xyz * attenuation * blinn_term;
  }

  // rgba
  out_color = vec4((diffuse_light + specular_light) * frag_color, 1.0);
}