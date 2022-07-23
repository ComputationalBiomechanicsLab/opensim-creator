#pragma once

// stolen from raylib: https://github.com/raysan5/raylib/blob/master/src/raylib.h
//
// the benefit of all shaders using the same locations is that the same VAO can be
// recycled

namespace osc
{
    enum ShaderLocationIndex {
        // vertex locations
        SHADER_LOC_VERTEX_POSITION = 0,
        SHADER_LOC_VERTEX_TEXCOORD01 = 1,
        SHADER_LOC_VERTEX_NORMAL = 2,
        SHADER_LOC_COLOR = 3,

        // instance locations
        SHADER_LOC_MATRIX_MODEL = 6,
        SHADER_LOC_MATRIX_NORMAL = 10,
        SHADER_LOC_COLOR_DIFFUSE = 13,
        SHADER_LOC_COLOR_RIM = 14,
    };
}
