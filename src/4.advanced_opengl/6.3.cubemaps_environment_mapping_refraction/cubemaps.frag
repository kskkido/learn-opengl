#version 330 core 
out vec4 FragColor;

in vec3 fNormal;
in vec3 fPosition;

uniform vec3 viewPosition;
uniform samplerCube texture1;

void main()
{    
  float ratio = 1.00 / 1.52;
  vec3 viewDirection = normalize(fPosition - viewPosition);
  vec3 reflection = refract(viewDirection, fNormal, ratio);
  FragColor = vec4(texture(texture1, reflection).rgb, 1.0);
}

