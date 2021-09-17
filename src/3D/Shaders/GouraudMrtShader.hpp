#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/Shader.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {

    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and labelling in COLOR1
    struct GouraudMrtShader final : public Shader {
        gl::Program program;

        // vertex attrs - the thing being instanced
        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;
        static constexpr gl::AttributeVec3 aNormal = SHADER_LOC_VERTEX_NORMAL;

        // instancing attrs - the instances - should be set with relevant divisor etc.
        static constexpr gl::AttributeMat4x3 aModelMat = SHADER_LOC_MATRIX_MODEL;
        static constexpr gl::AttributeMat3 aNormalMat = SHADER_LOC_MATRIX_NORMAL;
        static constexpr gl::AttributeVec4 aDiffuseColor = SHADER_LOC_COLOR_DIFFUSE;
        static constexpr gl::AttributeFloat aRimIntensity = SHADER_LOC_COLOR_RIM;

        // uniforms
        gl::UniformMat4 uProjMat;
        gl::UniformMat4 uViewMat;
        gl::UniformVec3 uLightDir;
        gl::UniformVec3 uLightColor;
        gl::UniformVec3 uViewPos;
        gl::UniformBool uIsTextured;
        gl::UniformSampler2D uSampler0;

        GouraudMrtShader();
    };
}
