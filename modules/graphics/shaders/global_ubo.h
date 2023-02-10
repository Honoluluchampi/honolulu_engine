struct PointLight
{
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo
{
  mat4 projection;
  mat4 view;
  mat4 inv_view;
  vec4 ambient_light_color;
  PointLight point_lights[20];
  int num_lights;
} ubo;