#pragma once

#include <glm/vec4.hpp>

#include <cstdint>

namespace osc
{
    struct Rgba32 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    struct Rgb24 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    // float-/double-based inputs assume linear color range (i.e. 0 to 1)
    Rgba32 Rgba32FromVec4(glm::vec4 const&) noexcept;
    Rgba32 Rgba32FromF4(float, float, float, float) noexcept;
    Rgba32 Rgba32FromU32(std::uint32_t) noexcept;  // R at MSB
    glm::vec4 GetSuggestedBoneColor() noexcept;  // best guess, based on shaders etc.
    glm::vec4 Roundoff(glm::vec4 const&);  // deterministically round off color values
}
