#pragma once

#include "src/3d/Gl.hpp"

namespace osc {

    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and labelling in COLOR1
    struct GouraudMrtShader final {
        gl::Program program;

        // vertex attrs - the thing being instanced
        static constexpr gl::AttributeVec3 aPos = 0;
        static constexpr gl::AttributeVec2 aTexCoord = 1;
        static constexpr gl::AttributeVec3 aNormal = 2;

        // instancing attrs - the instances - should be set with relevant divisor etc.
        static constexpr gl::AttributeMat4x3 aModelMat = 6;
        static constexpr gl::AttributeMat3 aNormalMat = 10;
        static constexpr gl::AttributeVec4 aDiffuseColor = 13;
        static constexpr gl::AttributeFloat aRimIntensity = 14;

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
