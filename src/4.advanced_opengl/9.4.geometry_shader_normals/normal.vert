#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinate;

out vec3 gNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
  mat3 normalMatrix = mat3(transpose(inverse(view * model)));
  gNormal = vec3(vec4(normalMatrix * aNormal, 0.0));
  gl_Position = view * model * vec4(aPosition, 1.0);
}
