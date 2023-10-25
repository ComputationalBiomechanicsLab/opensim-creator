#include "Variant.hpp"

#include <oscar_document/VariantType.hpp>

#include <glm/vec3.hpp>
#include <oscar/Bindings/GlmHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <charconv>
#include <cstddef>
#include <stdexcept>
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
        // TODO: temporarily using `std::strof` here, rather than `std::from_chars` (C++17),
        // because MacOS (Catalina) and Ubuntu 20 don't support the latter (as of Oct 2023)
        // for floating-point values

        std::string s{v};
        size_t pos = 0;
        try
        {
            return std::stof(s, &pos);
        }
        catch (std::exception const&)
        {
            return 0.0f;
        }
    }

    int ToIntOrZero(std::string_view v)
    {
        int rv{};
        auto [ptr, err] = std::from_chars(v.data(), v.data() + v.size(), rv);
        return err == std::errc{} ? rv : 0;
    }
}

osc::doc::Variant::Variant() : m_Data{std::monostate{}}
{
    static_assert(std::variant_size_v<decltype(m_Data)> == NumOptions<VariantType>());
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
        [&rv](std::monostate const&) { rv = osc::doc::VariantType::Nil; },
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

osc::doc::Variant::operator bool() const
{
    bool rv = false;
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = false; },
        [&rv](bool const& v) { rv = v; },
        [&rv](Color const& v) { rv = v.r != 0.0f; },
        [&rv](float const& v) { rv = v != 0.0f; },
        [&rv](int const& v) { rv = v != 0; },
        [&rv](std::string_view s) { rv = TryInterpretStringAsBool(s); },
        [&rv](glm::vec3 const& v) { rv = v.x != 0.0f; },
    }, m_Data);
    return rv;
}

osc::doc::Variant::operator osc::Color() const
{
    Color rv = Color::black();
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = Color::black(); },
        [&rv](bool const& v) { rv = v ? Color::white() : Color::black(); },
        [&rv](Color const& v) { rv = v; },
        [&rv](float const& v) { rv = Color{v, v, v}; },
        [&rv](int const& v) { rv = v ? Color::white() : Color::black(); },
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

osc::doc::Variant::operator float() const
{
    float rv = 0.0f;
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = 0.0f; },
        [&rv](bool const& v) { rv = v ? 1.0f : 0.0f; },
        [&rv](Color const& v) { rv = v.r; },
        [&rv](float const& v) { rv = v; },
        [&rv](int const& v) { rv = static_cast<float>(v); },
        [&rv](std::string_view s) { rv = ToFloatOrZero(s); },
        [&rv](glm::vec3 const& v) { rv = v.x; },
    }, m_Data);
    return rv;
}

osc::doc::Variant::operator int() const
{
    int rv = 0;
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = 0; },
        [&rv](bool const& v) { rv = v ? 1 : 0; },
        [&rv](Color const& v) { rv = static_cast<int>(v.r); },
        [&rv](float const& v) { rv = static_cast<int>(v); },
        [&rv](int const& v) { rv = v; },
        [&rv](std::string_view s) { rv = ToIntOrZero(s); },
        [&rv](glm::vec3 const& v) { rv = static_cast<int>(v.x); },
    }, m_Data);
    return rv;
}

osc::doc::Variant::operator std::string() const
{
    std::string rv;
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = "<null>"; },
        [&rv](bool const& v) { rv = v ? "true" : "false"; },
        [&rv](Color const& v) { rv = ToHtmlStringRGBA(v); },
        [&rv](float const& v) { rv = std::to_string(v); },
        [&rv](int const& v) { rv = std::to_string(v); },
        [&rv](std::string_view s) { rv = s; },
        [&rv](glm::vec3 const& v) { rv = osc::to_string(v); },
    }, m_Data);
    return rv;
}

osc::doc::Variant::operator osc::StringName() const
{
    StringName rv;
    std::visit(osc::Overload
    {
        [&rv](std::string const& s) { rv = StringName{s}; },
        [&rv](StringName const& sn) { rv = sn; },
        [](auto const&) {},
    }, m_Data);
    return rv;
}

osc::doc::Variant::operator glm::vec3() const
{
    glm::vec3 rv{};
    std::visit(osc::Overload
    {
        [&rv](std::monostate const&) { rv = glm::vec3{}; },
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
    // StringName vs. string is transparent w.r.t. comparison - even though they are
    // different entries in the std::variant

    if (std::holds_alternative<StringName>(lhs.m_Data) && std::holds_alternative<std::string>(rhs.m_Data))
    {
        return std::get<StringName>(lhs.m_Data) == std::get<std::string>(rhs.m_Data);
    }
    else if (std::holds_alternative<std::string>(lhs.m_Data) && std::holds_alternative<StringName>(rhs.m_Data))
    {
        return std::get<std::string>(lhs.m_Data) == std::get<StringName>(rhs.m_Data);
    }
    else
    {
        return lhs.m_Data == rhs.m_Data;
    }
}

bool osc::doc::operator!=(Variant const& lhs, Variant const& rhs)
{
    return !(lhs == rhs);
}

std::string osc::doc::to_string(Variant const& v)
{
    return v.to<std::string>();
}

std::ostream& osc::doc::operator<<(std::ostream& o, Variant const& v)
{
    return o << to_string(v);
}

size_t std::hash<osc::doc::Variant>::operator()(osc::doc::Variant const& v) const
{
    return std::hash<decltype(osc::doc::Variant::m_Data)>{}(v.m_Data);
}
