#version 330 core
layout (location = 0) in vec3 aPosition;

uniform mat4 perspective;
uniform mat4 view;

void main()
{
  gl_Position = perspective * view * vec4(aPosition, 1.0);
}
