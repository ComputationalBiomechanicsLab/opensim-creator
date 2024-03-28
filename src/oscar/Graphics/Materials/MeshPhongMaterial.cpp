#include "MeshPhongMaterial.h"

#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Utils/CStringView.h>

using namespace osc;

namespace
{
    constexpr CStringView c_vertex_shader_src = R"(
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
    constexpr CStringView c_fragment_shader_src = R"(
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
    m_Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{
    set_light_position({1.0f, 1.0f, 1.0f});
    set_viewer_position({0.0f, 0.0f, 0.0f});
    set_light_color(Color::white());
    set_ambient_color({0.1f, 0.1f, 0.1f});
    set_diffuse_color(Color::blue());
    set_specular_color({0.1f, 0.1f, 0.1f});
    set_specular_shininess(32.0f);
}
