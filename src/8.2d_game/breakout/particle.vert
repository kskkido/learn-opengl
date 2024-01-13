#version 330 core
layout (location = 0) in vec2 aPosition; // <vec2 position, vec2 texCoords>
layout (location = 1) in vec2 aTextureCoordinate; // <vec2 position, vec2 texCoords>
layout (location = 2) in vec2 aOffset;
layout (location = 3) in vec4 aColor;

out vec2 TexCoords;
out vec4 ParticleColor;
uniform mat4 projection;

void main() {
  float scale = 10.0f;
  TexCoords = aTextureCoordinate;
  ParticleColor = aColor;
  gl_Position = projection * vec4((aPosition * scale) + aOffset, 0.0, 1.0);
}
