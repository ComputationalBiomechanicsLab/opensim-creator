#include "MeshBasicMaterial.h"

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Utils/CStringView.h>

using namespace osc;

namespace
{
    constexpr CStringView c_VertexShaderSrc = R"(
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
    constexpr CStringView c_FragmentShaderSrc = R"(
#version 330 core

uniform vec4 uDiffuseColor;
out vec4 FragColor;

void main()
{
    FragColor = uDiffuseColor;
}
)";
}

osc::MeshBasicMaterial::MeshBasicMaterial() :
    m_Material{Shader{c_VertexShaderSrc, c_FragmentShaderSrc}}
{
    setColor(Color::black());
}
