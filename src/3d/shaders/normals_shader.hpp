#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program;

        static constexpr gl::Attribute_vec3 aPos = 0;
        static constexpr gl::Attribute_vec3 aNormal = 1;

        gl::Uniform_mat4 uModelMat;
        gl::Uniform_mat4 uViewMat;
        gl::Uniform_mat4 uProjMat;
        gl::Uniform_mat4 uNormalMat;

        Normals_shader();
    };
}
