#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program;

        gl::Attribute_vec3 aPos;
        gl::Attribute_vec3 aNormal;

        gl::Uniform_mat4 uModelMat;
        gl::Uniform_mat4 uViewMat;
        gl::Uniform_mat4 uProjMat;
        gl::Uniform_mat4 uNormalMat;

        Normals_shader();
    };
}
