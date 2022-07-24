#version 330 core

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform vec3 uObjectColor;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform float uSpecularStrength;

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

void main()
{
    // ambient
    vec3 ambient = uAmbientStrength * uLightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uDiffuseStrength * diff * uLightColor;

    // specular
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = uSpecularStrength * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    FragColor = vec4(result, 1.0);
}