#include "Variant.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
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
        else if (is_equal_case_insensitive(s, "false"))
        {
            return false;
        }
        else if (is_equal_case_insensitive(s, "0"))
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
        // HACK: temporarily using `std::strof` here, rather than `std::from_chars` (C++17),
        // because MacOS (Catalina) and Ubuntu 20 don't support the latter (as of Oct 2023)
        // for floating-point values

        std::string s{v};
        size_t pos = 0;
        try
        {
            return std::stof(s, &pos);
        }
        catch (const std::exception&)
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
    static_assert(std::variant_size_v<decltype(m_Data)> == num_options<VariantType>());
}
osc::Variant::Variant(bool v) : m_Data{v} {}
osc::Variant::Variant(Color v) : m_Data{v} {}
osc::Variant::Variant(float v) : m_Data{v} {}
osc::Variant::Variant(int v) : m_Data{v} {}
osc::Variant::Variant(std::string v) : m_Data{std::move(v)} {}
osc::Variant::Variant(std::string_view v) : m_Data{std::string{v}} {}
osc::Variant::Variant(const StringName& v) : m_Data{v} {}
osc::Variant::Variant(Vec2 v) : m_Data{v} {}
osc::Variant::Variant(Vec3 v) : m_Data{v} {}

VariantType osc::Variant::type() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return VariantType::Nil; },
        [](const bool&) { return VariantType::Bool; },
        [](const Color&) { return VariantType::Color; },
        [](const float&) { return VariantType::Float; },
        [](const int&) { return VariantType::Int; },
        [](const std::string&) { return VariantType::String; },
        [](const StringName&) { return VariantType::StringName; },
        [](const Vec2&) { return VariantType::Vec2; },
        [](const Vec3&) { return VariantType::Vec3; },
    }, m_Data);
}

osc::Variant::operator bool() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return false; },
        [](const bool& v) { return v; },
        [](const Color& v) { return v.r != 0.0f; },
        [](const float& v) { return v != 0.0f; },
        [](const int& v) { return v != 0; },
        [](std::string_view s) { return TryInterpretStringAsBool(s); },
        [](const Vec2& v) { return v.x != 0.0f; },
        [](const Vec3& v) { return v.x != 0.0f; },
    }, m_Data);
}

osc::Variant::operator osc::Color() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return Color::black(); },
        [](const bool& v) { return v ? Color::white() : Color::black(); },
        [](const Color& v) { return v; },
        [](const float& v) { return Color{v}; },
        [](const int& v) { return v ? Color::white() : Color::black(); },
        [](std::string_view s)
        {
            const auto c = try_parse_html_color_string(s);
            return c ? *c : Color::black();
        },
        [](const Vec2& v) { return Color{v.x, v.y, 0.0f}; },
        [](const Vec3& v) { return Color{v}; },
    }, m_Data);
}

osc::Variant::operator float() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return 0.0f; },
        [](const bool& v) { return v ? 1.0f : 0.0f; },
        [](const Color& v) { return v.r; },
        [](const float& v) { return v; },
        [](const int& v) { return static_cast<float>(v); },
        [](std::string_view s) { return ToFloatOrZero(s); },
        [](const Vec2& v) { return v.x; },
        [](const Vec3& v) { return v.x; },
    }, m_Data);
}

osc::Variant::operator int() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return 0; },
        [](const bool& v) { return v ? 1 : 0; },
        [](const Color& v) { return static_cast<int>(v.r); },
        [](const float& v) { return static_cast<int>(v); },
        [](const int& v) { return v; },
        [](std::string_view s) { return ToIntOrZero(s); },
        [](const Vec2& v) { return static_cast<int>(v.x); },
        [](const Vec3& v) { return static_cast<int>(v.x); },
    }, m_Data);
}

osc::Variant::operator std::string() const
{
    using namespace std::literals::string_literals;

    return std::visit(Overload
    {
        [](const std::monostate&) { return "<null>"s; },
        [](const bool& v) { return v ? "true"s : "false"s; },
        [](const Color& v) { return to_html_string_rgba(v); },
        [](const float& v) { return std::to_string(v); },
        [](const int& v) { return std::to_string(v); },
        [](std::string_view s) { return std::string{s}; },
        [](const Vec2& v) { return to_string(v); },
        [](const Vec3& v) { return to_string(v); },
    }, m_Data);
}

osc::Variant::operator osc::StringName() const
{
    return std::visit(Overload
    {
        [](const std::string& s) { return StringName{s}; },
        [](const StringName& sn) { return sn; },
        [](const auto&) { return StringName{}; },
    }, m_Data);
}

osc::Variant::operator osc::Vec2() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return Vec2{}; },
        [](const bool& v) { return v ? Vec2{1.0f, 1.0f} : Vec2{}; },
        [](const Color& v) { return Vec2{v.r, v.g}; },
        [](const float& v) { return Vec2{v}; },
        [](const int& v) { return Vec2{static_cast<float>(v)}; },
        [](std::string_view) { return Vec2{}; },
        [](const Vec2& v) { return v; },
        [](const Vec3& v) { return Vec2{v}; },
    }, m_Data);
}

osc::Variant::operator osc::Vec3() const
{
    return std::visit(Overload
    {
        [](const std::monostate&) { return Vec3{}; },
        [](const bool& v) { return v ? Vec3{1.0f, 1.0f, 1.0f} : Vec3{}; },
        [](const Color& v) { return Vec3{v.r, v.g, v.b}; },
        [](const float& v) { return Vec3{v}; },
        [](const int& v) { return Vec3{static_cast<float>(v)}; },
        [](std::string_view) { return Vec3{}; },
        [](const Vec2& v) { return Vec3{v, 0.0f}; },
        [](const Vec3& v) { return v; },
    }, m_Data);
}

bool osc::operator==(const Variant& lhs, const Variant& rhs)
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

std::string osc::to_string(const Variant& v)
{
    return v.to<std::string>();
}

std::ostream& osc::operator<<(std::ostream& o, const Variant& v)
{
    return o << to_string(v);
}

size_t std::hash<osc::Variant>::operator()(const Variant& v) const
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
    //     hash_of(Variant) == hash_of(std::string) == hash_of(std::string_view) == hash_of(StringName)...

    return std::visit(Overload
    {
        [](const auto& inner)
        {
            return std::hash<std::remove_cv_t<std::remove_reference_t<decltype(inner)>>>{}(inner);
        },
    }, v.m_Data);
}
