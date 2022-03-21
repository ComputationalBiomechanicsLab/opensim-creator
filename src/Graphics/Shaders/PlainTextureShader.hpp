#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"

namespace osc
{
    struct PlainTextureShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;

        gl::UniformMat4 uMVP;
        gl::UniformFloat uTextureScaler;
        gl::UniformSampler2D uSampler0;

        PlainTextureShader();
    };
}
