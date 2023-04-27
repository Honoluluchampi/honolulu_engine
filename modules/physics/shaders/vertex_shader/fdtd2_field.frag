#version 450

layout (location = 0) out vec4 out_color;

void main()
{
  out_color = vec4(gl_FragCoord.x, gl_FragCoord.y, 0, 1);
}