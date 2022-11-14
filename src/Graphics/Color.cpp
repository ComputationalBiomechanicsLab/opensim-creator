#include "Color.hpp"

#include <glm/glm/glm.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

osc::Rgba32 osc::ToRgba32(glm::vec4 const& v) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>(255.0f * v.r),
        static_cast<uint8_t>(255.0f * v.g),
        static_cast<uint8_t>(255.0f * v.b),
        static_cast<uint8_t>(255.0f * v.a),
    };
}

osc::Rgba32 osc::ToRgba32(float r, float g, float b, float a) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>(255.0f * r),
        static_cast<uint8_t>(255.0f * g),
        static_cast<uint8_t>(255.0f * b),
        static_cast<uint8_t>(255.0f * a),
    };
}

osc::Rgba32 osc::ToRgba32(std::uint32_t v) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>((v >> 24) & 0xff),
        static_cast<uint8_t>((v >> 16) & 0xff),
        static_cast<uint8_t>((v >> 8) & 0xff),
        static_cast<uint8_t>((v >> 0) & 0xff),
    };
}