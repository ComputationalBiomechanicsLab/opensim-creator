#version 330 core

uniform mat4 uViewProjMat;
uniform mat4 uLightSpaceMat;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform vec2 uTextureScale = vec2(1.0, 1.0);
uniform float uDiffuseStrength;
uniform float uSpecularStrength;
uniform float uShininess;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 6) in mat4 aModelMat;
layout (location = 10) in mat3 aNormalMat;

out vec3 FragWorldPos;
out vec4 FragLightSpacePos;
out vec3 NormalWorldDir;
out float NonAmbientBrightness;
out vec2 TexCoord;

void main()
{
    vec3 normalDir = normalize(aNormalMat * aNormal);
    vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
    vec3 frag2viewDir = normalize(uViewPos - fragPos);
    vec3 frag2lightDir = normalize(-uLightDir);
    vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

    // care: these lighting calculations use "double-sided normals", because
    // mesh data from users can have screwed normals/winding, but OSC still
    // should try its best to render it "correct enough" (#168, #318)
    float diffuseAmt = uDiffuseStrength * abs(dot(normalDir, frag2lightDir));
    float specularAmt = uSpecularStrength * pow(abs(dot(normalDir, halfwayDir)), uShininess);

    vec4 worldPos = aModelMat * vec4(aPos, 1.0);

    FragWorldPos = vec3(worldPos);
    FragLightSpacePos = uLightSpaceMat * vec4(FragWorldPos, 1.0);
    NormalWorldDir = normalDir;
    NonAmbientBrightness = diffuseAmt + specularAmt;
    TexCoord = uTextureScale * aTexCoord;

    gl_Position = uViewProjMat * worldPos;
}
