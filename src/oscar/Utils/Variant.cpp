#include "Variant.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Utils/VariantHelpers.hpp>
#include <oscar/Utils/VariantType.hpp>

#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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

osc::Variant::Variant() : m_Data{std::monostate{}}
{
    static_assert(std::variant_size_v<decltype(m_Data)> == NumOptions<VariantType>());
}
osc::Variant::Variant(bool v) : m_Data{v} {}
osc::Variant::Variant(Color v) : m_Data{v} {}
osc::Variant::Variant(float v) : m_Data{v} {}
osc::Variant::Variant(int v) : m_Data{v} {}
osc::Variant::Variant(std::string v) : m_Data{std::move(v)} {}
osc::Variant::Variant(std::string_view v) : m_Data{std::string{v}} {}
osc::Variant::Variant(char const* v) : m_Data{std::string{v}} {}
osc::Variant::Variant(CStringView v) : m_Data{std::string{v}} {}
osc::Variant::Variant(StringName const& v) : m_Data{v} {}
osc::Variant::Variant(Vec3 v) : m_Data{v} {}

osc::VariantType osc::Variant::getType() const
{
    VariantType rv = VariantType::Bool;
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = VariantType::Nil; },
        [&rv](bool const&) { rv = VariantType::Bool; },
        [&rv](Color const&) { rv = VariantType::Color; },
        [&rv](float const&) { rv = VariantType::Float; },
        [&rv](int const&) { rv = VariantType::Int; },
        [&rv](std::string const&) { rv = VariantType::String; },
        [&rv](StringName const&) { rv = VariantType::StringName; },
        [&rv](Vec3 const&) { rv = VariantType::Vec3; },
    }, m_Data);
    return rv;
}

osc::Variant::operator bool() const
{
    bool rv = false;
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = false; },
        [&rv](bool const& v) { rv = v; },
        [&rv](Color const& v) { rv = v.r != 0.0f; },
        [&rv](float const& v) { rv = v != 0.0f; },
        [&rv](int const& v) { rv = v != 0; },
        [&rv](std::string_view s) { rv = TryInterpretStringAsBool(s); },
        [&rv](Vec3 const& v) { rv = v.x != 0.0f; },
    }, m_Data);
    return rv;
}

osc::Variant::operator osc::Color() const
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
        [&rv](Vec3 const& v) { rv = osc::Color{v}; },
    }, m_Data);
    return rv;
}

osc::Variant::operator float() const
{
    float rv = 0.0f;
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = 0.0f; },
        [&rv](bool const& v) { rv = v ? 1.0f : 0.0f; },
        [&rv](Color const& v) { rv = v.r; },
        [&rv](float const& v) { rv = v; },
        [&rv](int const& v) { rv = static_cast<float>(v); },
        [&rv](std::string_view s) { rv = ToFloatOrZero(s); },
        [&rv](Vec3 const& v) { rv = v.x; },
    }, m_Data);
    return rv;
}

osc::Variant::operator int() const
{
    int rv = 0;
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = 0; },
        [&rv](bool const& v) { rv = v ? 1 : 0; },
        [&rv](Color const& v) { rv = static_cast<int>(v.r); },
        [&rv](float const& v) { rv = static_cast<int>(v); },
        [&rv](int const& v) { rv = v; },
        [&rv](std::string_view s) { rv = ToIntOrZero(s); },
        [&rv](Vec3 const& v) { rv = static_cast<int>(v.x); },
    }, m_Data);
    return rv;
}

osc::Variant::operator std::string() const
{
    std::string rv;
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = "<null>"; },
        [&rv](bool const& v) { rv = v ? "true" : "false"; },
        [&rv](Color const& v) { rv = ToHtmlStringRGBA(v); },
        [&rv](float const& v) { rv = std::to_string(v); },
        [&rv](int const& v) { rv = std::to_string(v); },
        [&rv](std::string_view s) { rv = s; },
        [&rv](Vec3 const& v) { rv = to_string(v); },
    }, m_Data);
    return rv;
}

osc::Variant::operator osc::StringName() const
{
    StringName rv;
    std::visit(Overload
    {
        [&rv](std::string const& s) { rv = StringName{s}; },
        [&rv](StringName const& sn) { rv = sn; },
        [](auto const&) {},
    }, m_Data);
    return rv;
}

osc::Variant::operator osc::Vec3() const
{
    Vec3 rv{};
    std::visit(Overload
    {
        [&rv](std::monostate const&) { rv = Vec3{}; },
        [&rv](bool const& v) { rv = v ? Vec3{1.0f, 1.0f, 1.0f} : Vec3{}; },
        [&rv](Color const& v) { rv = {v.r, v.g, v.b}; },
        [&rv](float const& v) { rv = {v, v, v}; },
        [&rv](int const& v)
        {
            auto const fv = static_cast<float>(v);
            rv = {fv, fv, fv};
        },
        [&rv](std::string_view) { rv = Vec3{}; },
        [&rv](Vec3 const& v) { rv = v; },
    }, m_Data);
    return rv;
}

bool osc::operator==(Variant const& lhs, Variant const& rhs)
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

bool osc::operator!=(Variant const& lhs, Variant const& rhs)
{
    return !(lhs == rhs);
}

std::string osc::to_string(Variant const& v)
{
    return v.to<std::string>();
}

std::ostream& osc::operator<<(std::ostream& o, Variant const& v)
{
    return o << to_string(v);
}

size_t std::hash<osc::Variant>::operator()(osc::Variant const& v) const
{
    // note: you might be wondering why this isn't `std::hash<std::variant>{}(v.m_Data)`
    //
    // > cppreference.com
    // >
    // > Unlike std::hash<std::optional>, hash of a variant does not typically equal the
    // > hash of the contained value; this makes it possible to distinguish std::variant<int, int>
    // > holding the same value as different alternatives.
    //
    // but osc's Variant doesn't need to support this edge-case, and transparent hashing of
    // the contents can be handy when you want behavior like:
    //
    //     HashOf(Variant) == HashOf(std::string) == HashOf(std::string_view) == HashOf(StringName)...

    size_t rv = 0;
    std::visit(osc::Overload
    {
        [&rv](auto const& inner)
        {
            rv = std::hash<std::remove_cv_t<std::remove_reference_t<decltype(inner)>>>{}(inner);
        },
    }, v.m_Data);
    return rv;
}
