#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {
    struct InstancedGouraudColorShader final {
        gl::Program program;

        // vertex attrs - the thing being instanced
        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec3 aNormal = SHADER_LOC_VERTEX_NORMAL;

        // instancing attrs - the instances - should be set with relevant divisor etc.
        static constexpr gl::AttributeMat4x3 aModelMat = SHADER_LOC_MATRIX_MODEL;
        static constexpr gl::AttributeMat3 aNormalMat = SHADER_LOC_MATRIX_NORMAL;
        static constexpr gl::AttributeVec4 aDiffuseColor = SHADER_LOC_COLOR_DIFFUSE;

        // uniforms
        gl::UniformMat4 uProjMat;
        gl::UniformMat4 uViewMat;
        gl::UniformVec3 uLightDir;
        gl::UniformVec3 uLightColor;
        gl::UniformVec3 uViewPos;

        InstancedGouraudColorShader();
    };
}
