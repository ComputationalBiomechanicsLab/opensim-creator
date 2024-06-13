#include "MeshBasicMaterial.h"

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Utils/CStringView.h>

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
}

osc::MeshBasicMaterial::MeshBasicMaterial(const Color& color) :
    material_{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{
    set_color(color);
}
