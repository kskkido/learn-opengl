#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

in vec3 gColor[];

out vec3 fColor;

void draw(vec4 position, vec3 color)
{
  fColor = color;
  gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0);
  EmitVertex();
  gl_Position = position + vec4(0.2, -0.2, 0.0, 0.0);
  EmitVertex();
  gl_Position = position + vec4(-0.2, 0.2, 0.0, 0.0);
  EmitVertex();
  gl_Position = position + vec4(0.2, 0.2, 0.0, 0.0);
  EmitVertex();
  gl_Position = position + vec4(0.0, 0.4, 0.0, 0.0);
  fColor = vec3(1.0, 1.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

void main() {
  draw(gl_in[0].gl_Position, gColor[0]);
}
