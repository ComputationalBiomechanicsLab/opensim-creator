#include "Variant.hpp"

#include <oscar_document/VariantType.hpp>

#include <glm/vec3.hpp>
#include <oscar/Bindings/GlmHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace
{
    bool TryInterpretStringAsBool(std::string_view s)
    {
        if (s.empty())
        {
            return false;
        }
        else if (osc::IsEqualCaseInsensitive(s, "false"))
        {
            return false;
        }
        else if (osc::IsEqualCaseInsensitive(s, "0"))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    float ToFloatOrZero(std::string_view v)
    {
        float rv{};
        auto [ptr, err] = std::from_chars(v.data(), v.data() + v.size(), rv);
        return err == std::errc{} ? rv : 0.0f;
    }

    int ToIntOrZero(std::string_view v)
    {
        int rv{};
        auto [ptr, err] = std::from_chars(v.data(), v.data() + v.size(), rv);
        return err == std::errc{} ? rv : 0;
    }
}

osc::doc::Variant::Variant(bool v) : m_Data{v} {}
osc::doc::Variant::Variant(Color v) : m_Data{v} {}
osc::doc::Variant::Variant(float v) : m_Data{v} {}
osc::doc::Variant::Variant(int v) : m_Data{v} {}
osc::doc::Variant::Variant(std::string v) : m_Data{std::move(v)} {}
osc::doc::Variant::Variant(std::string_view v) : m_Data{std::string{v}} {}
osc::doc::Variant::Variant(char const* v) : m_Data{std::string{v}} {}
osc::doc::Variant::Variant(CStringView v) : m_Data{std::string{v}} {}
osc::doc::Variant::Variant(StringName const& v) : m_Data{v} {}
osc::doc::Variant::Variant(glm::vec3 v) : m_Data{v} {}

osc::doc::VariantType osc::doc::Variant::getType() const
{
    osc::doc::VariantType rv = osc::doc::VariantType::Bool;
    std::visit(osc::Overload
    {
        [&rv](bool const&) { rv = osc::doc::VariantType::Bool; },
        [&rv](Color const&) { rv = osc::doc::VariantType::Color; },
        [&rv](float const&) { rv = osc::doc::VariantType::Float; },
        [&rv](int const&) { rv = osc::doc::VariantType::Int; },
        [&rv](std::string const&) { rv = osc::doc::VariantType::String; },
        [&rv](StringName const&) { rv = osc::doc::VariantType::StringName; },
        [&rv](glm::vec3 const&) { rv = osc::doc::VariantType::Vec3; },
    }, m_Data);
    return rv;
}

bool osc::doc::Variant::toBool() const
{
    bool rv = false;
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v; },
        [&rv](Color const& v) { rv = v.r != 0.0f; },
        [&rv](float const& v) { rv = v != 0.0f; },
        [&rv](int const& v) { rv = v != 0; },
        [&rv](std::string_view s) { rv = TryInterpretStringAsBool(s); },
        [&rv](glm::vec3 const& v) { rv = v.x != 0.0f; },
    }, m_Data);
    return rv;
}

osc::Color osc::doc::Variant::toColor() const
{
    osc::Color rv = osc::Color::black();
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v ? osc::Color::white() : osc::Color::black(); },
        [&rv](Color const& v) { rv = v; },
        [&rv](float const& v) { rv = osc::Color{v, v, v}; },
        [&rv](int const& v) { rv = v ? osc::Color::white() : osc::Color::black(); },
        [&rv](std::string_view s)
        {
            if (auto c = TryParseHtmlString(s))
            {
                rv = *c;
            }
        },
        [&rv](glm::vec3 const& v) { rv = osc::Color{v}; },
    }, m_Data);
    return rv;
}

float osc::doc::Variant::toFloat() const
{
    float rv = 0.0f;
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v ? 1.0f : 0.0f; },
        [&rv](Color const& v) { rv = v.r; },
        [&rv](float const& v) { rv = v; },
        [&rv](int const& v) { rv = static_cast<float>(v); },
        [&rv](std::string_view s) { rv = ToFloatOrZero(s); },
        [&rv](glm::vec3 const& v) { rv = v.x; },
    }, m_Data);
    return rv;
}

int osc::doc::Variant::toInt() const
{
    int rv = 0;
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v ? 1 : 0; },
        [&rv](Color const& v) { rv = static_cast<int>(v.r); },
        [&rv](float const& v) { rv = static_cast<int>(v); },
        [&rv](int const& v) { rv = v; },
        [&rv](std::string_view s) { rv = ToIntOrZero(s); },
        [&rv](glm::vec3 const& v) { rv = static_cast<int>(v.x); },
    }, m_Data);
    return rv;
}

std::string osc::doc::Variant::toString() const
{
    std::string rv;
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v ? "true" : "false"; },
        [&rv](Color const& v) { rv = ToHtmlStringRGBA(v); },
        [&rv](float const& v) { rv = std::to_string(v); },
        [&rv](int const& v) { rv = std::to_string(v); },
        [&rv](std::string_view s) { rv = s; },
        [&rv](glm::vec3 const& v) { rv = osc::to_string(v); },
    }, m_Data);
    return rv;
}

glm::vec3 osc::doc::Variant::toVec3() const
{
    glm::vec3 rv{};
    std::visit(osc::Overload
    {
        [&rv](bool const& v) { rv = v ? glm::vec3{1.0f, 1.0f, 1.0f} : glm::vec3{}; },
        [&rv](Color const& v) { rv = {v.r, v.g, v.b}; },
        [&rv](float const& v) { rv = {v, v, v}; },
        [&rv](int const& v) { float fv = static_cast<float>(v); rv = {fv, fv, fv}; },
        [&rv](std::string_view) { rv = glm::vec3{}; },
        [&rv](glm::vec3 const& v) { rv = v; },
    }, m_Data);
    return rv;
}

bool osc::doc::operator==(Variant const& lhs, Variant const& rhs)
{
    return lhs.m_Data == rhs.m_Data;
}

bool osc::doc::operator!=(Variant const& lhs, Variant const& rhs)
{
    return lhs.m_Data != rhs.m_Data;
}

size_t std::hash<osc::doc::Variant>::operator()(osc::doc::Variant const& v) const
{
    return std::hash<decltype(osc::doc::Variant::m_Data)>{}(v.m_Data);
}
