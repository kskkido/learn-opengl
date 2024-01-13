#version 330 core
struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

struct Light {
  vec3 direction;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 position;
  float constant;
  float linear;
  float quadratic;
};

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragmentPosition;

out vec4 FragColor;
  
uniform Material material;
uniform Light light;
uniform vec3 viewPosition;
uniform vec3 objectColor;

void main()
{
    // attenuation
    float d = length(light.position - FragmentPosition);
    float attenuation = 1.0 / (light.constant + light.linear * d + light.quadratic * (d * d));
      // ambient
    vec3 ambient = attenuation * light.ambient * texture(material.diffuse, TexCoords).rgb;
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = attenuation * light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;  
    // specular
    vec3 viewDir = normalize(viewPosition - FragmentPosition);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = attenuation * light.specular * spec * texture(material.specular, TexCoords).rgb;  
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
