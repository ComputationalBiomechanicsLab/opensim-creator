#pragma once

#include "src/3d/gl.hpp"

namespace osc {
    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and labelling in COLOR1
    struct Gouraud_mrt_shader final {
        gl::Program program;

        // vertex attrs - the thing being instanced
        gl::Attribute_vec3 aLocation;
        gl::Attribute_vec3 aNormal;
        gl::Attribute_vec2 aTexCoord;

        // instancing attrs - the instances - should be set with relevant divisor etc.
        gl::Attribute_mat4x3 aModelMat;
        gl::Attribute_mat3 aNormalMat;
        gl::Attribute_vec4 aRgba0;
        gl::Attribute_float aRimIntensity;

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
