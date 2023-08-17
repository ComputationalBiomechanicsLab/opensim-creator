#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D uPositionTex;
uniform sampler2D uNormalTex;
uniform sampler2D uAlbedoTex;
uniform sampler2D uSSAOTex;
uniform vec3 uLightPosition;
uniform vec3 uLightColor;
uniform float uLightLinear;
uniform float uLightQuadratic;

void main()
{
    // retrieve data from gbuffer
    vec3 FragPos = texture(uPositionTex, TexCoords).rgb;
    vec3 Normal = texture(uNormalTex, TexCoords).rgb;
    vec3 Diffuse = texture(uAlbedoTex, TexCoords).rgb;
    float AmbientOcclusion = texture(uSSAOTex, TexCoords).r;

    // then calculate lighting as usual
    vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
    vec3 lighting  = ambient;
    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0)

    // diffuse
    vec3 lightDir = normalize(uLightPosition - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLightColor;

    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = uLightColor * spec;

    // attenuation
    float frag2light = length(uLightPosition - FragPos);
    float attenuation = 1.0 / (1.0 + (uLightLinear * frag2light) + (uLightQuadratic * frag2light*frag2light));

    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;

    FragColor = vec4(lighting, 1.0);
}
