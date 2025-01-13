#pragma once

// the benefit of all shaders using the same locations is that the same VAO can be
// recycled
namespace osc::shader_locations
{
    static inline constexpr int aPos = 0;
    static inline constexpr int aTexCoord = 1;
    static inline constexpr int aNormal = 2;
    static inline constexpr int aColor = 3;
    static inline constexpr int aTangent = 4;
    static inline constexpr int aModelMat = 6;
    static inline constexpr int aNormalMat = 10;
    static inline constexpr int aDiffuseColor = 13;
    static inline constexpr int aRimColor = 14;
}
