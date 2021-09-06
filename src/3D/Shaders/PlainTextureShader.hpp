#pragma once

#include "src/3D/Gl.hpp"

namespace osc {
    struct PlainTextureShader final {
        gl::Program program;

        static constexpr gl::AttributeVec3 aPos = 0;
        static constexpr gl::AttributeVec2 aTexCoord = 1;

        gl::UniformMat4 uMVP;
        gl::UniformFloat uTextureScaler;
        gl::UniformSampler2D uSampler0;

        PlainTextureShader();
    };
}
