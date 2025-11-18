#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;
uniform mat3 uNormalMat;
uniform bool uInvertedNormals;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

void main()
{
    vec4 viewPos = uViewMat * uModelMat * vec4(aPos, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uViewMat * uModelMat)));

    FragPos = viewPos.xyz;
    TexCoords = aTexCoord;
    Normal = normalMatrix * (uInvertedNormals ? -aNormal : aNormal);
    gl_Position = uProjMat * viewPos;
}
