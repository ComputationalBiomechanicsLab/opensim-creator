#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/Shader.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {

    struct GouraudShader final : public Shader {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;
        static constexpr gl::AttributeVec3 aNormal = SHADER_LOC_VERTEX_NORMAL;

        // uniforms
        gl::UniformMat4 uProjMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uModelMat;
        gl::UniformMat3 uNormalMat;
        gl::UniformVec4 uDiffuseColor;
        gl::UniformVec3 uLightDir;
        gl::UniformVec3 uLightColor;
        gl::UniformVec3 uViewPos;
        gl::UniformBool uIsTextured;
        gl::UniformSampler2D uSampler0;

        GouraudShader();
    };
}
