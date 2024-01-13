#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTextureCoordinate;

out vec2 fTextureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  fTextureCoordinate = aTextureCoordinate;
  gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
