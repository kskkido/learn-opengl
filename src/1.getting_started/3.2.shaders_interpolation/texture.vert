#version 330 core
layout (location = 0) in vec3 aPos; // position has attribute position 0
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

void main() {
  gl_Position = vec4(aPos, 1.0); // we give a vec3 to vec4’s constructor
  ourColor = aColor;
}

