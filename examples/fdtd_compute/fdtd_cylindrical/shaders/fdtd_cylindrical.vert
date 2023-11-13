#version 450

vec2 positions[6] = {
  vec2(-1, -1),
  vec2(1, -1),
  vec2(-1, 1),
  vec2(1, -1),
  vec2(-1, 1),
  vec2(1, 1)
};

void main()
{
  uint id = gl_VertexIndex;
  gl_Position = vec4(positions[id].x, positions[id].y, 0, 0);
}