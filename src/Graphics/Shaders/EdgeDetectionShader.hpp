#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"

namespace osc
{
    // A specialized edge-detection shader for rim highlighting
    struct EdgeDetectionShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;

        gl::UniformMat4 uMVP;
        gl::UniformSampler2D uSampler0;
        gl::UniformVec4 uRimRgba;
        gl::UniformVec2 uRimThickness;

        EdgeDetectionShader();
    };
}
