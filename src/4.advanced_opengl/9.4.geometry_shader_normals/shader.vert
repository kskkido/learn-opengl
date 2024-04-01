#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinate;

out vec2 fTextureCoordinate;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
  fTextureCoordinate = aTextureCoordinate;
  gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
