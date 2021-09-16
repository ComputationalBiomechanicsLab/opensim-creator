#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

namespace osc {
    // A specialized edge-detection shader for rim highlighting
    struct EdgeDetectionShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = SHADER_LOC_VERTEX_POSITION;
        static constexpr gl::AttributeVec2 aTexCoord = SHADER_LOC_VERTEX_TEXCOORD01;

        gl::UniformMat4 uModelMat;
        gl::UniformMat4 uViewMat;
        gl::UniformMat4 uProjMat;
        gl::UniformSampler2D uSampler0;
        gl::UniformVec4 uRimRgba;
        gl::UniformFloat uRimThickness;

        EdgeDetectionShader();
    };
}
