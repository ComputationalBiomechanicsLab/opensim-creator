#pragma once

#include "src/3D/Gl.hpp"

namespace osc {
    // uses a geometry shader to render normals as lines
    struct NormalsShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = 0;
        static constexpr gl::AttributeVec3 aNormal = 2;

        gl::UniformMat4 uModelMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uProjMat;
        gl::UniformMat4 uNormalMat;

        NormalsShader();
    };
}
