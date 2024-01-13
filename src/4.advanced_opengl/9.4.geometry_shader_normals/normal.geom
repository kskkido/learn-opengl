#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in vec3 gNormal[];

uniform mat4 projection;

void drawLine(vec4 position, vec3 normal, float magnitude)
{
    gl_Position = projection * position;
    EmitVertex();
    gl_Position = projection * (position + vec4(normal, 0.0) * magnitude);
    EmitVertex();
    EndPrimitive();
}

void main()
{
  drawLine(gl_in[0].gl_Position, gNormal[0], 0.2); // first vertex normal
  drawLine(gl_in[1].gl_Position, gNormal[1], 0.2); // second vertex normal
  drawLine(gl_in[2].gl_Position, gNormal[2], 0.2); // third vertex normal
}
