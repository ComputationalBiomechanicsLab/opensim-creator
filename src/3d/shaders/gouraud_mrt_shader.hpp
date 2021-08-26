#pragma once

#include "src/3d/gl.hpp"

namespace osc {

    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and labelling in COLOR1
    struct Gouraud_mrt_shader final {
        gl::Program program;

        // vertex attrs - the thing being instanced
        static constexpr gl::Attribute_vec3 aLocation = 0;
        static constexpr gl::Attribute_vec3 aNormal = 1;
        static constexpr gl::Attribute_vec2 aTexCoord = 2;

        // instancing attrs - the instances - should be set with relevant divisor etc.
        static constexpr gl::Attribute_mat4x3 aModelMat = 3;
        static constexpr gl::Attribute_mat3 aNormalMat = 7;
        static constexpr gl::Attribute_vec4 aRgba0 = 10;
        static constexpr gl::Attribute_float aRimIntensity = 11;

        // uniforms
        gl::Uniform_mat4 uProjMat;
        gl::Uniform_mat4 uViewMat;
        gl::Uniform_vec3 uLightDir;
        gl::Uniform_vec3 uLightColor;
        gl::Uniform_vec3 uViewPos;
        gl::Uniform_bool uIsTextured;
        gl::Uniform_sampler2d uSampler0;

        Gouraud_mrt_shader();
    };
}
