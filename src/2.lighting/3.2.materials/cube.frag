#version 330 core
struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in vec3 Normal;
in vec3 FragmentPosition;

out vec4 FragColor;
  
uniform Material material;
uniform Light light;
uniform vec3 viewPosition;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(light.position - FragmentPosition);
    vec3 diffuse = light.diffuse * (max(dot(norm, lightDirection), 0.0) * material.diffuse);
    vec3 viewDirection = normalize(viewPosition - FragmentPosition);
    vec3 specular = light.specular * (pow(max(dot(norm, reflect(-lightDirection, norm)), 0.0), material.shininess) * material.specular);
    vec3 ambient = light.ambient * material.ambient;
    FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}
