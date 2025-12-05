#version 330 core

const int NR_LIGHTS = 32;

uniform sampler2D uPositionTex;
uniform sampler2D uNormalTex;
uniform sampler2D uAlbedoTex;
uniform vec3 uLightPositions[NR_LIGHTS];
uniform vec3 uLightColors[NR_LIGHTS];
uniform float uLightLinear;
uniform float uLightQuadratic;
uniform vec3 uViewPos;

in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    // retrieve data from gbuffer
    vec3 FragPos = texture(uPositionTex, TexCoord).rgb;
    vec3 Normal = texture(uNormalTex, TexCoord).rgb;
    vec3 Diffuse = texture(uAlbedoTex, TexCoord).rgb;
    float Specular = texture(uAlbedoTex, TexCoord).a;

    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(uViewPos - FragPos);
    for (int i = 0; i < NR_LIGHTS; ++i)
    {
        // diffuse
        vec3 lightDir = normalize(uLightPositions[i] - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLightColors[i];

        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = uLightColors[i] * spec * Specular;

        // attenuation
        float distance = length(uLightPositions[i] - FragPos);
        float attenuation = 1.0 / (1.0 + uLightLinear * distance + uLightQuadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;
    }
    FragColor = vec4(lighting, 1.0);
}
