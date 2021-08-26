#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    // A basic shader that just samples a texture onto the provided geometry
    //
    // useful for rendering quads etc.
    struct Colormapped_plain_texture_shader final {
        gl::Program p;

        static constexpr gl::Attribute_vec3 aPos = 0;
        static constexpr gl::Attribute_vec2 aTexCoord = 1;

        gl::Uniform_mat4 uMVP;
        gl::Uniform_sampler2d uSampler0;
        gl::Uniform_mat4 uSamplerMultiplier;

        Colormapped_plain_texture_shader();
    };
}
