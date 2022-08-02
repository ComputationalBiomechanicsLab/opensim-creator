#version 330 core

const float ambientStrength = 0.3f;
const float diffuseStrength = 0.8f;
const float specularStrength = 0.7f;
const float shininess = 64;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

in vec3 FragPosWorld;
in vec3 NormalWorld;

out vec4 FragColor;

void main()
{
    vec3 light2fragDir = normalize(uLightDir);
    vec3 normalDir = normalize(NormalWorld);
    vec3 frag2viewDir = normalize(uViewPos - FragPosWorld);
    vec3 frag2lightDir = -light2fragDir;
    vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

    // blinn shading
    float ambientAmt = ambientStrength;
    float diffuseAmt = diffuseStrength * abs(dot(normalDir, frag2lightDir));
    float specularAmt = specularStrength * pow(abs(dot(normalDir, halfwayDir)), shininess);

    float lightAmt = ambientAmt + diffuseAmt + specularAmt;

    FragColor = uDiffuseColor * vec4(lightAmt * uLightColor, 1.0);
}