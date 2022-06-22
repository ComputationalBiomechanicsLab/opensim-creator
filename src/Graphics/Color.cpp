#include "Color.hpp"

#include <glm/glm/glm.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

osc::Rgba32 osc::Rgba32FromVec4(glm::vec4 const& v) noexcept
{
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * v.r);
    rv.g = static_cast<unsigned char>(255.0f * v.g);
    rv.b = static_cast<unsigned char>(255.0f * v.b);
    rv.a = static_cast<unsigned char>(255.0f * v.a);
    return rv;
}

osc::Rgba32 osc::Rgba32FromF4(float r, float g, float b, float a) noexcept
{
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * r);
    rv.g = static_cast<unsigned char>(255.0f * g);
    rv.b = static_cast<unsigned char>(255.0f * b);
    rv.a = static_cast<unsigned char>(255.0f * a);
    return rv;
}

osc::Rgba32 osc::Rgba32FromU32(std::uint32_t v) noexcept
{
    Rgba32 rv;
    rv.r = static_cast<unsigned char>((v >> 24) & 0xff);
    rv.g = static_cast<unsigned char>((v >> 16) & 0xff);
    rv.b = static_cast<unsigned char>((v >> 8) & 0xff);
    rv.a = static_cast<unsigned char>((v >> 0) & 0xff);
    return rv;
}

glm::vec4 osc::GetSuggestedBoneColor() noexcept
{
    glm::vec4 usualDefault = {232.0f / 255.0f, 216.0f / 255.0f, 200.0f/255.0f, 1.0f};
    glm::vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
    float brightenAmount = 0.1f;
    return glm::mix(usualDefault, white, brightenAmount);
}
