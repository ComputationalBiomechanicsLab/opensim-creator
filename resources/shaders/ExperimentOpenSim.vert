#version 330 core

uniform mat4 uViewProjMat;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 6) in mat4 aModelMat;
layout (location = 10) in mat3 aNormalMat;

out vec4 GouraudBrightness;

const float ambientStrength = 0.3f;
const float diffuseStrength = 0.8f;
const float specularStrength = 0.7f;
const float shininess = 64;

void main()
{
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);

    vec3 normalDir = normalize(aNormalMat * aNormal);
    vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
    vec3 frag2viewDir = normalize(uViewPos - fragPos);
    vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction
    vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

    float ambientAmt = ambientStrength;
    float diffuseAmt = diffuseStrength * abs(dot(normalDir, frag2lightDir));
    float specularAmt = specularStrength * pow(abs(dot(normalDir, halfwayDir)), shininess);

    float lightAmt = clamp(ambientAmt + diffuseAmt + specularAmt, 0.0, 1.0);

    GouraudBrightness = vec4(lightAmt * uLightColor, 1.0);
}