#version 330 core

uniform vec3 uSceneLightPositions[4];
uniform vec3 uSceneLightColors[4];
uniform sampler2D uDiffuseTexture;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    vec3 color = texture(uDiffuseTexture, TexCoords).rgb;
    vec3 normal = normalize(Normal);

    // ambient
    vec3 ambient = 0.0 * color;

    // lighting
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < 4; i++)
    {
        // diffuse
        vec3 lightDir = normalize(uSceneLightPositions[i] - FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = uSceneLightColors[i] * diff * color;
        vec3 result = diffuse;
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(FragPos - uSceneLightPositions[i]);
        result *= 1.0 / (distance * distance);

        lighting += result;
    }

    FragColor = vec4(ambient + lighting, 1.0);
}
