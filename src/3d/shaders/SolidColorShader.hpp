#pragma once

#include "src/3d/Gl.hpp"

namespace osc {
    struct SolidColorShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = 0;

        gl::UniformMat4 uModel;
        gl::UniformMat4 uView;
        gl::UniformMat4 uProjection;
        gl::UniformVec4 uColor;

        SolidColorShader();
    };
}
