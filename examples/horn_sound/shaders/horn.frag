#version 450

layout (location = 0) out vec4 out_color;

struct horn_push {
  float h_length; // horn
  float h_width;
  float w_width; // window
  float w_height;
};

layout(push_constant) uniform Push { horn_push push; };

// 3 pixel width line
const float eps = 3.f;

void main() {
  bool on_length = abs(gl_FragCoord.x - w_width / 2.f) < push.h_length / 2.f;
  bool on_width  = abs(gl_FragCoord.y - w_height / 2.f) < eps;

  out_color = vec4(vec3(1.f, 1.f, 1.f) * on_length * on_width, 1.f);
}