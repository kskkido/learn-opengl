#version 330 core
layout (location = 0) in vec3 aPosition;

out vec3 fTextureCoordinate;

uniform mat4 view;
uniform mat4 projection;

void main()
{
  fTextureCoordinate = aPosition;
  vec4 position = projection * view * vec4(aPosition, 1.0);
  gl_Position = position.xyww;
}
