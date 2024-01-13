#version 330 core
struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 direction;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float cutOff;
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
  vec3 direction = normalize(light.position - FragmentPosition);
  float theta = dot(direction, normalize(-light.direction));
  if (theta > light.cutOff)
  { 
    // attenuation
    float distance = length(light.position - FragmentPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
      // ambient
    vec3 ambient = attenuation * light.ambient * texture(material.diffuse, TexCoords).rgb;
    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, direction), 0.0);
    vec3 diffuse = attenuation * light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;  
    // specular
    vec3 viewDir = normalize(viewPosition - FragmentPosition);
    vec3 reflectDir = reflect(-direction, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = attenuation * light.specular * spec * texture(material.specular, TexCoords).rgb;  
        
    FragColor = vec4(ambient + diffuse + specular, 1.0);
  }
  else 
  {
    FragColor = vec4(light.ambient * texture(material.diffuse, TexCoords).rgb, 1.0);
  }
}
