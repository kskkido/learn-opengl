#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in mat4 aModel;

out vec3 fColor;

uniform mat4 projection;
uniform mat4 view;

void main()
{
  fColor = aColor;
  gl_Position = projection * view * aModel * vec4(aPosition, 1.0);
}
