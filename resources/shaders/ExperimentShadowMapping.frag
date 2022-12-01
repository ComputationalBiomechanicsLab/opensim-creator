#version 330 core

uniform vec3 uLightColor;
uniform vec3 uLightWorldPos;
uniform vec3 uViewWorldPos;
uniform sampler2D uDiffuseTexture;

in vec3 FragWorldPos;
in vec3 NormalWorldDir;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    vec3 frag2LightWorldDir = normalize(uLightWorldPos - FragWorldPos);

    float ambientAmt = 0.3;
    float diffuseAmt = max(dot(frag2LightWorldDir, NormalWorldDir), 0.0);
    float specularAmt = 0.0;
    {
        vec3 frag2ViewWorldDir = normalize(uLightWorldPos - FragWorldPos);
        vec3 halfwayDir = normalize(frag2LightWorldDir + frag2ViewWorldDir);
        specularAmt = pow(max(dot(NormalWorldDir, halfwayDir), 0.0), 64.0);
    }

    float brightness = ambientAmt + diffuseAmt + specularAmt;
    vec3 diffuseColor = texture(uDiffuseTexture, TexCoord).rgb;

    // TODO: shadow calculation

    FragColor = vec4(brightness * diffuseColor, 1.0);
}