#include "MeshBasicMaterial.h"

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>

using namespace osc;

namespace
{
    constexpr CStringView c_vertex_shader_src = R"(
#version 330 core

uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 6) in mat4 aModelMat;

void main()
{
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);
}
)";
    constexpr CStringView c_fragment_shader_src = R"(
#version 330 core

uniform vec4 uDiffuseColor;
out vec4 FragColor;

void main()
{
    FragColor = uDiffuseColor;
}
)";

    const StringName& color_property_name()
    {
        static const StringName s_color_property_name{"uDiffuseColor"};
        return s_color_property_name;
    }
}

osc::MeshBasicMaterial::PropertyBlock::PropertyBlock(const Color& color)
{
    set_color(color);
}

std::optional<Color> osc::MeshBasicMaterial::PropertyBlock::color() const
{
    return get<Color>(color_property_name());
}

void osc::MeshBasicMaterial::PropertyBlock::set_color(const Color& c)
{
    set(color_property_name(), c);
}

osc::MeshBasicMaterial::MeshBasicMaterial(const Params& p) :
    Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{
    set_color(p.color);
}

osc::MeshBasicMaterial::MeshBasicMaterial(const Color& color) :
    MeshBasicMaterial{Params{.color = color}}
{}

Color osc::MeshBasicMaterial::color() const
{
    return *get<Color>(color_property_name());
}

void osc::MeshBasicMaterial::set_color(const Color& color)
{
    set(color_property_name(), color);
}
