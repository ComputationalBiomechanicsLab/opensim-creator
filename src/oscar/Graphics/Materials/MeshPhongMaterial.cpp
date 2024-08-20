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

    const StringName c_light_pos_propname{"uLightPos"};
    const StringName c_view_pos_propname{"uViewPos"};
    const StringName c_light_color_propname{"uLightColor"};
    const StringName c_ambient_color_propname{"uAmbientColor"};
    const StringName c_diffuse_color_propname{"uDiffuseColor"};
    const StringName c_specular_color_propname{"uSpecularColor"};
    const StringName c_shininess_propname{"uShininess"};
}


osc::MeshPhongMaterial::MeshPhongMaterial(const Params& p) :
    Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{
    set_light_position(p.light_position);
    set_viewer_position(p.viewer_position);
    set_light_color(p.light_color);
    set_ambient_color(p.ambient_color);
    set_diffuse_color(p.diffuse_color);
    set_specular_color(p.specular_color);
    set_specular_shininess(p.specular_shininess);
}

Vec3 osc::MeshPhongMaterial::light_position() const { return *get<Vec3>(c_light_pos_propname); }
void osc::MeshPhongMaterial::set_light_position(const Vec3& v) { set(c_light_pos_propname, v); }

Vec3 osc::MeshPhongMaterial::viewer_position() const { return *get<Vec3>(c_view_pos_propname); }
void osc::MeshPhongMaterial::set_viewer_position(const Vec3& v) { set(c_view_pos_propname, v); }

Color osc::MeshPhongMaterial::light_color() const { return *get<Color>(c_light_color_propname); }
void osc::MeshPhongMaterial::set_light_color(const Color& c) { set(c_light_color_propname, c); }

Color osc::MeshPhongMaterial::ambient_color() const { return *get<Color>(c_ambient_color_propname); }
void osc::MeshPhongMaterial::set_ambient_color(const Color& c) { set(c_ambient_color_propname, c); }

Color osc::MeshPhongMaterial::diffuse_color() const { return *get<Color>(c_diffuse_color_propname); }
void osc::MeshPhongMaterial::set_diffuse_color(const Color& c) { set(c_diffuse_color_propname, c); }

Color osc::MeshPhongMaterial::specular_color() const { return *get<Color>(c_specular_color_propname); }
void osc::MeshPhongMaterial::set_specular_color(const Color& c) { set(c_specular_color_propname, c); }

float osc::MeshPhongMaterial::specular_shininess() const { return *get<float>(c_shininess_propname); }
void osc::MeshPhongMaterial::set_specular_shininess(float v) { set(c_shininess_propname, v); }
