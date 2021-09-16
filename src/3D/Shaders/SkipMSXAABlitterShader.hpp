#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {
    // designed to blit the first sample from a multisampled texture source
    struct SkipMSXAABlitterShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;

        gl::UniformMat4 uModelMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uProjMat;
        gl::UniformSampler2DMS uSampler0;

        SkipMSXAABlitterShader();
    };
}
