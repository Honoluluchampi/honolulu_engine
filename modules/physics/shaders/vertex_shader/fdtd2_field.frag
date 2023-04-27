#version 450

layout (location = 0) out vec4 out_color;

struct fdtd2_frag_push {
  float width;
  float height;
};

layout (push_constant) uniform Push {
  fdtd2_frag_push push;
};

void main()
{
  out_color = vec4(gl_FragCoord.x / push.width, gl_FragCoord.y / push.height, 0, 1);
}