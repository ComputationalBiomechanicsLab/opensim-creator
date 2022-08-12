#version 330 core

uniform mat4 uViewProjMat;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform vec2 uTextureScale = vec2(1.0, 1.0);
uniform float uAmbientStrength = 0.7f;
uniform float uDiffuseStrength = 0.4f;
uniform float uSpecularStrength = 0.4f;
uniform float uShininess = 8;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 6) in mat4 aModelMat;
layout (location = 10) in mat3 aNormalMat;

out vec2 TexCoord;
out vec4 GouraudBrightness;

void main()
{
    vec3 normalDir = normalize(aNormalMat * aNormal);
    vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
    vec3 frag2viewDir = normalize(uViewPos - fragPos);
    vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction
    vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

    float ambientAmt = uAmbientStrength;
    float diffuseAmt = uDiffuseStrength * abs(dot(normalDir, frag2lightDir));
    float specularAmt = uSpecularStrength * pow(abs(dot(normalDir, halfwayDir)), uShininess);

    float lightAmt = clamp(ambientAmt + diffuseAmt + specularAmt, 0.0, 1.0);

    GouraudBrightness = vec4(lightAmt * uLightColor, 1.0);
    TexCoord = uTextureScale * aTexCoord;
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);
}
