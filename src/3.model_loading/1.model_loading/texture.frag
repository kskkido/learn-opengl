#version 330 core
struct SpotLight {
  vec3 position;
  vec3 direction;
  float cutOff;
  float outerCutOff;
  float constant;
  float linear;
  float quadratic;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;       
};

out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragmentPosition;

uniform SpotLight spotLight;
uniform vec3 viewPosition;
uniform sampler2D texture_diffuse1;

vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection)
{
  vec3 lightDirection = normalize(light.position - fragmentPosition);
  // diffuse shading
  float diffuse = max(dot(normal, lightDirection), 0.0);
  // specular shading
  vec3 reflectDirection = reflect(-lightDirection, normal);
  float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), 1.0);
  // attenuation
  float distance = length(light.position - fragmentPosition);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
  // spotlight intensity
  float theta = dot(lightDirection, normalize(-light.direction));
  float epsilon = light.cutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  // combine results
  vec3 outAmbient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));
  vec3 outDiffuse = light.diffuse * diffuse * vec3(texture(texture_diffuse1, TexCoords));
  vec3 outSpecular = light.specular * specular * vec3(texture(texture_diffuse1, TexCoords));
  outAmbient *= attenuation * intensity;
  outDiffuse *= attenuation * intensity;
  outSpecular *= attenuation * intensity;
  return (outAmbient + outDiffuse + outSpecular);
}

void main()
{    
  vec3 normal = normalize(Normal);
  vec3 viewDirection = normalize(viewPosition - FragmentPosition);
  vec3 result = calculateSpotLight(spotLight, normal, FragmentPosition, viewDirection);
  FragColor = vec4(result, 1.0);
}
