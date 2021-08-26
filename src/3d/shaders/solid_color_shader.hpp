#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    struct Solid_color_shader final {
        gl::Program prog;

        static constexpr gl::Attribute_vec3 aPos = 0;

        gl::Uniform_mat4 uModel;
        gl::Uniform_mat4 uView;
        gl::Uniform_mat4 uProjection;
        gl::Uniform_vec4 uColor;

        Solid_color_shader();
    };
}
