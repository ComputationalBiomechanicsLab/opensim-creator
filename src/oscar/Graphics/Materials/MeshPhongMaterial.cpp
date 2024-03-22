#include "MeshPhongMaterial.h"

#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Utils/CStringView.h>

using namespace osc;

namespace
{
    constexpr CStringView c_VertexShaderSrc = R"(
#version 330 core

uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(uModelMat * vec4(aPos, 1.0));
    Normal = uNormalMat * aNormal;

    gl_Position = uViewProjMat * vec4(FragPos, 1.0);
}
)";
    constexpr CStringView c_FragmentShaderSrc = R"(
#version 330 core

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec4 uLightColor;
uniform vec4 uAmbientColor;
uniform vec4 uDiffuseColor;
uniform vec4 uSpecularColor;
uniform float uShininess;

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

void main()
{
    // ambient
    vec3 ambient = vec3(uAmbientColor) * vec3(uLightColor);

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(uDiffuseColor) * vec3(uLightColor);

    // specular
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3 specular = spec * vec3(uSpecularColor) * vec3(uLightColor);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
}

osc::MeshPhongMaterial::MeshPhongMaterial() :
    m_Material{Shader{c_VertexShaderSrc, c_FragmentShaderSrc}}
{
    setLightPosition({1.0f, 1.0f, 1.0f});
    setViewerPosition({0.0f, 0.0f, 0.0f});
    setLightColor(Color::white());
    setAmbientColor({0.1f, 0.1f, 0.1f});
    setDiffuseColor(Color::blue());
    setSpecularColor({0.1f, 0.1f, 0.1f});
    setSpecularShininess(32.0f);
}
