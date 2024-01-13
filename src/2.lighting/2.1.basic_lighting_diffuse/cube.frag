#version 330 core
in vec3 Normal;
in vec3 FragmentPosition;

out vec4 FragColor;
  
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPosition - FragmentPosition);
    vec3 diffuse = max(dot(norm, lightDirection), 0.0) * lightColor;
    vec3 ambient = 0.1 * lightColor;
    FragColor = vec4((ambient + diffuse) * objectColor, 1.0);
}
