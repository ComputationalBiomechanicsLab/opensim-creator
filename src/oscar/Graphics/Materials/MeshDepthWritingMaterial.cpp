#include "MeshDepthWritingMaterial.h"

#include <string_view>

using namespace osc;

namespace
{
    constexpr std::string_view c_vertex_shader_src = R"(
#version 330 core

uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 6) in mat4 aModelMat;

void main()
{
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);
}
)";
    constexpr std::string_view c_fragment_shader_src = R"(
#version 330 core

void main() {}  // implicitly writes the depth
)";
}

osc::MeshDepthWritingMaterial::MeshDepthWritingMaterial()
    : Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{}
