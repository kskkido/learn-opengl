#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

out vec3 fNormal;
out vec3 fPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  fNormal = mat3(transpose(inverse(model))) * aNormal;
  fPosition = vec3(model * vec4(aPosition, 1.0));
  gl_Position = projection * view * vec4(fPosition, 1.0);
}
