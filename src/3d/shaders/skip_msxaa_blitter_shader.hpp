#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    // designed to blit the first sample from a multisampled texture source
    struct Skip_msxaa_blitter_shader final {
        gl::Program p;

        static constexpr gl::Attribute_vec3 aPos = 0;
        static constexpr gl::Attribute_vec2 aTexCoord = 1;

        gl::Uniform_mat4 uModelMat;
        gl::Uniform_mat4 uViewMat;
        gl::Uniform_mat4 uProjMat;
        gl::Uniform_sampler2DMS uSampler0;

        Skip_msxaa_blitter_shader();
    };
}
