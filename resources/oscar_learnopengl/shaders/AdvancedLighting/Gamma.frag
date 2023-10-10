#version 330 core

uniform sampler2D uFloorTexture;
uniform vec3 uLightPositions[4];
uniform vec4 uLightColors[4];
uniform vec3 uViewPos;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

vec3 BlinnPhong(
    vec3 normal,
    vec3 fragPos,
    vec3 lightPos,
    vec3 lightColor)
{
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(uViewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // simple attenuation
    float max_distance = 1.5;
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance*distance);

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse + specular;
}

void main()
{
    vec3 color = texture(uFloorTexture, TexCoord).rgb;

    // accumulate light from all scene lights
    vec3 brightness = vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        brightness += BlinnPhong(
            normalize(Normal),
            FragPos,
            uLightPositions[i],
            vec3(uLightColors[i])
        );
    }
    color *= brightness;

    FragColor = vec4(color, 1.0);
}
