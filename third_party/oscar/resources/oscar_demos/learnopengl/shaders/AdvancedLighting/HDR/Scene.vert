#version 330 core

uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform mat4 uViewProjMat;
uniform bool uInverseNormals = false;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    vec4 pos = uModelMat * vec4(aPos, 1.0);
    vec3 normal = uInverseNormals ? -aNormal : aNormal;

    FragPos = vec3(pos);
    Normal = uNormalMat * normal;
    TexCoords = aTexCoords;
    gl_Position = uViewProjMat * pos;
}
