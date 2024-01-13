#version 330 core
in vec2 fTexCoords;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
  FragColor = texture(texture1, fTexCoords);
  FragColor = vec4(0.2126, 0.7152, 0.0722, 1.0) * FragColor;
  float average = FragColor.r + FragColor.g + FragColor.b;
  FragColor = vec4(average, average, average, 1.0); 
}
