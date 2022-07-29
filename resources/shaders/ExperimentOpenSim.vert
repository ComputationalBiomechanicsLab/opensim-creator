#version 330 core

uniform mat4 uViewProjMat;
uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out vec4 GouraudBrightness;

const float ambientStrength = 0.5f;
const float diffuseStrength = 0.5f;
const float specularStrength = 0.7f;
const float shininess = 32;

void main()
{
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);

    vec3 normalDir = normalize(uNormalMat * aNormal);
    vec3 fragPos = vec3(uModelMat * vec4(aPos, 1.0));
    vec3 frag2viewDir = normalize(uViewPos - fragPos);
    vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction
    vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

    float ambientAmt = ambientStrength;
    float diffuseAmt = diffuseStrength * max(dot(normalDir, frag2lightDir), 0.0);
    float specularAmt = specularStrength * pow(max(dot(normalDir, halfwayDir), 0.0), shininess);

    float lightAmt = clamp(ambientAmt + diffuseAmt + specularAmt, 0.0, 1.0);

    GouraudBrightness = vec4(lightAmt * uLightColor, 1.0);
}