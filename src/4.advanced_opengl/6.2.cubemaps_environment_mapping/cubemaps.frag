#version 330 core 
out vec4 FragColor;

in vec3 fNormal;
in vec3 fPosition;

uniform vec3 viewPosition;
uniform samplerCube texture1;

void main()
{    
  vec3 viewDirection = normalize(fPosition - viewPosition);
  vec3 reflection = reflect(viewDirection, fNormal);
  FragColor = vec4(texture(texture1, reflection).rgb, 1.0);
}

