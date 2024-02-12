#include "Variant.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Variant/VariantType.h>

#include <charconv>
#include <cstddef>
#include <exception>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

using namespace osc;

namespace
{
    bool TryInterpretStringAsBool(std::string_view s)
    {
        if (s.empty())
        {
            return false;
        }
        else if (IsEqualCaseInsensitive(s, "false"))
        {
            return false;
        }
        else if (IsEqualCaseInsensitive(s, "0"))
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
osc::Variant::Variant(StringName const& v) : m_Data{v} {}
osc::Variant::Variant(Vec3 v) : m_Data{v} {}

VariantType osc::Variant::getType() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return VariantType::Nil; },
        [](bool const&) { return VariantType::Bool; },
        [](Color const&) { return VariantType::Color; },
        [](float const&) { return VariantType::Float; },
        [](int const&) { return VariantType::Int; },
        [](std::string const&) { return VariantType::String; },
        [](StringName const&) { return VariantType::StringName; },
        [](Vec3 const&) { return VariantType::Vec3; },
    }, m_Data);
}

osc::Variant::operator bool() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return false; },
        [](bool const& v) { return v; },
        [](Color const& v) { return v.r != 0.0f; },
        [](float const& v) { return v != 0.0f; },
        [](int const& v) { return v != 0; },
        [](std::string_view s) { return TryInterpretStringAsBool(s); },
        [](Vec3 const& v) { return v.x != 0.0f; },
    }, m_Data);
}

osc::Variant::operator osc::Color() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return Color::black(); },
        [](bool const& v) { return v ? Color::white() : Color::black(); },
        [](Color const& v) { return v; },
        [](float const& v) { return Color{v}; },
        [](int const& v) { return v ? Color::white() : Color::black(); },
        [](std::string_view s)
        {
            auto const c = TryParseHtmlString(s);
            return c ? *c : Color::black();
        },
        [](Vec3 const& v) { return Color{v}; },
    }, m_Data);
}

osc::Variant::operator float() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return 0.0f; },
        [](bool const& v) { return v ? 1.0f : 0.0f; },
        [](Color const& v) { return v.r; },
        [](float const& v) { return v; },
        [](int const& v) { return static_cast<float>(v); },
        [](std::string_view s) { return ToFloatOrZero(s); },
        [](Vec3 const& v) { return v.x; },
    }, m_Data);
}

osc::Variant::operator int() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return 0; },
        [](bool const& v) { return v ? 1 : 0; },
        [](Color const& v) { return static_cast<int>(v.r); },
        [](float const& v) { return static_cast<int>(v); },
        [](int const& v) { return v; },
        [](std::string_view s) { return ToIntOrZero(s); },
        [](Vec3 const& v) { return static_cast<int>(v.x); },
    }, m_Data);
}

osc::Variant::operator std::string() const
{
    using namespace std::literals::string_literals;

    return std::visit(Overload
    {
        [](std::monostate const&) { return "<null>"s; },
        [](bool const& v) { return v ? "true"s : "false"s; },
        [](Color const& v) { return ToHtmlStringRGBA(v); },
        [](float const& v) { return std::to_string(v); },
        [](int const& v) { return std::to_string(v); },
        [](std::string_view s) { return std::string{s}; },
        [](Vec3 const& v) { return to_string(v); },
    }, m_Data);
}

osc::Variant::operator osc::StringName() const
{
    return std::visit(Overload
    {
        [](std::string const& s) { return StringName{s}; },
        [](StringName const& sn) { return sn; },
        [](auto const&) { return StringName{}; },
    }, m_Data);
}

osc::Variant::operator osc::Vec3() const
{
    return std::visit(Overload
    {
        [](std::monostate const&) { return Vec3{}; },
        [](bool const& v) { return v ? Vec3{1.0f, 1.0f, 1.0f} : Vec3{}; },
        [](Color const& v) { return Vec3{v.r, v.g, v.b}; },
        [](float const& v) { return Vec3{v}; },
        [](int const& v) { return Vec3{static_cast<float>(v)}; },
        [](std::string_view) { return Vec3{}; },
        [](Vec3 const& v) { return v; },
    }, m_Data);
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

std::string osc::to_string(Variant const& v)
{
    return v.to<std::string>();
}

std::ostream& osc::operator<<(std::ostream& o, Variant const& v)
{
    return o << to_string(v);
}

size_t std::hash<osc::Variant>::operator()(Variant const& v) const
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

    return std::visit(Overload
    {
        [](auto const& inner)
        {
            return std::hash<std::remove_cv_t<std::remove_reference_t<decltype(inner)>>>{}(inner);
        },
    }, v.m_Data);
}
