#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"

namespace osc
{
    struct InstancedSolidColorShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeMat4x3 aModelMat = SHADER_LOC_MATRIX_MODEL;

        gl::UniformMat4 uVP;
        gl::UniformVec4 uColor;

        InstancedSolidColorShader();
    };
}
