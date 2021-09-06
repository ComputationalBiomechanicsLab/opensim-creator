#pragma once

#include "src/3d/Gl.hpp"

namespace osc {
    // designed to blit the first sample from a multisampled texture source
    struct SkipMSXAABlitterShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = 0;
        static constexpr gl::AttributeVec2 aTexCoord = 1;

        gl::UniformMat4 uModelMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uProjMat;
        gl::UniformSampler2DMS uSampler0;

        SkipMSXAABlitterShader();
    };
}
