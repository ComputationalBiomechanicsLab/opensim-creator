#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    struct Plain_texture_shader final {
        gl::Program p;

        gl::Attribute_vec3 aPos;
        gl::Attribute_vec2 aTexCoord;

        gl::Uniform_mat4 uMVP;
        gl::Uniform_float uTextureScaler;
        gl::Uniform_sampler2d uSampler0;

        Plain_texture_shader();
    };
}
