#pragma once

#include "src/3D/Gl.hpp"

namespace osc {
    // A specialized edge-detection shader for rim highlighting
    struct EdgeDetectionShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = 0;
        static constexpr gl::AttributeVec2 aTexCoord = 1;

        gl::UniformMat4 uModelMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uProjMat;
        gl::UniformSampler2D uSampler0;
        gl::UniformVec4 uRimRgba;
        gl::UniformFloat uRimThickness;

        EdgeDetectionShader();
    };
}
