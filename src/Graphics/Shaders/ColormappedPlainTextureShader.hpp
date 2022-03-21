#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"

namespace osc
{
    // A basic shader that just samples a texture onto the provided geometry
    //
    // useful for rendering quads etc.
    struct ColormappedPlainTextureShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;

        gl::UniformMat4 uMVP;
        gl::UniformSampler2D uSamplerAlbedo;
        gl::UniformMat4 uSamplerMultiplier;

        ColormappedPlainTextureShader();
    };
}
