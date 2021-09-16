#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {
    struct SolidColorShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;

        gl::UniformMat4 uModel;
        gl::UniformMat4 uView;
        gl::UniformMat4 uProjection;
        gl::UniformVec4 uColor;

        SolidColorShader();
    };
}
