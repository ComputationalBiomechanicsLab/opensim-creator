#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"

namespace osc
{
    struct SolidColorShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;

        gl::UniformMat4 uModel;
        gl::UniformMat4 uView;
        gl::UniformMat4 uProjection;
        gl::UniformVec4 uColor;

        SolidColorShader();
    };
}
