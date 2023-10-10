#version 330 core

uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform mat4 uViewProjMat;
uniform bool uReverseNormals;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 fragAffineWorldPos = uModelMat * vec4(aPos, 1.0);

    TexCoords = aTexCoord;
    FragPos = vec3(fragAffineWorldPos);
    Normal = uNormalMat * (uReverseNormals ? -aNormal : aNormal);
    gl_Position = uViewProjMat * fragAffineWorldPos;
}
