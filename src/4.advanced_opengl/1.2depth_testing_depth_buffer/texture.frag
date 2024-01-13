#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;

float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{    
  float near = 0.1; 
  float far = 50.0; 
  float depth = LinearizeDepth(gl_FragCoord.z, near, far) / far; // divide by far to get depth in range [0,1] for visualization purposes
  FragColor = vec4(vec3(depth), 1.0);
}
