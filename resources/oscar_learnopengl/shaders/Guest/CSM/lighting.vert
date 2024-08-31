#version 330 core

const int NUM_CASCADES = 3;

uniform mat4 uViewProjMat;
uniform mat4 uLightWVP[NUM_CASCADES];

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 6) in mat4 aModelMat;
layout (location = 10) in mat3 aNormalMat;

out vec4 LightSpacePos[NUM_CASCADES];
out float ClipSpacePosZ;
out vec2 TexCoord0;
out vec3 Normal0;
out vec3 WorldPos0;

void main()
{
    vec4 Pos = vec4(aPos, 1.0);

    gl_Position = uViewProjMat * aModelMat * Pos;
    
    for (int i = 0 ; i < NUM_CASCADES ; i++) {
        LightSpacePos[i] = uLightWVP[i] * Pos;
    }
    ClipSpacePosZ = gl_Position.z;
    TexCoord0     = aTexCoord;
    Normal0       = aNormalMat * aNormal;
    WorldPos0     = vec3(aModelMat * Pos);
}
