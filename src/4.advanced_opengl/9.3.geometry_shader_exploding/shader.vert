#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinate;

out vec2 gTextureCoordinate;

uniform mat4 perspective;
uniform mat4 view;
uniform mat4 model;

void main()
{
  gTextureCoordinate = aTextureCoordinate;
  gl_Position = perspective * view * model * vec4(aPosition, 1.0);
}
