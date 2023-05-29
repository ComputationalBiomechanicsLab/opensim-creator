#version 330 core

uniform vec3 uLightPositions[4];
uniform vec4 uLightColors[4];
uniform sampler2D uDiffuseTexture;
uniform vec3 uViewWorldPos;

in vec3 FragWorldPos;
in vec3 NormalWorld;
in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragFilteredColor;

void main()
{           
    vec3 diffuseColor = texture(uDiffuseTexture, TexCoord).rgb;
    vec3 normalWorldDir = normalize(NormalWorld);
    vec3 frag2ViewWorldDir = normalize(uViewWorldPos - FragWorldPos);

    vec3 totalLightColor = vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        vec3 frag2LightWorld =  uLightPositions[i] - FragWorldPos;
        float frag2LightWorldDistance = length(frag2LightWorld);
        vec3 frag2LightWorldDir = frag2LightWorld / frag2LightWorldDistance;
        
        float diffuseBrightness = max(dot(frag2LightWorldDir, normalWorldDir), 0.0);
        vec3 unattenuatedColor = vec3(uLightColors[i]) * diffuseBrightness * diffuseColor;
        float attenuation = 1.0/(frag2LightWorldDistance*frag2LightWorldDistance);

        totalLightColor += (attenuation * unattenuatedColor);
    }

    FragColor = vec4(totalLightColor, 1.0);

    // thresholded write
    if (dot(totalLightColor, vec3(0.2126, 0.7152, 0.0722)) > 1.0)
    {
        FragFilteredColor = vec4(totalLightColor, 1.0);
    }
    else
    {
        FragFilteredColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}