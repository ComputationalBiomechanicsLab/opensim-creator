#pragma once

#include "src/Graphics/Rgba32.hpp"

#include <glm/vec4.hpp>

#include <cstdint>

namespace osc
{
    // float-/double-based inputs assume linear color range (i.e. 0 to 1)
    Rgba32 ToRgba32(glm::vec4 const&) noexcept;
    Rgba32 ToRgba32(float, float, float, float) noexcept;
    Rgba32 ToRgba32(uint32_t) noexcept;  // R at MSB
}
