#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {
    struct InstancedSolidColorShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeMat4x3 aModelMat = SHADER_LOC_MATRIX_MODEL;

        gl::UniformMat4 uVP;
        gl::UniformVec4 uColor;

        InstancedSolidColorShader();
    };
}
