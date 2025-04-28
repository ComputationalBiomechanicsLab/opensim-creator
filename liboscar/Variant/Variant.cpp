#include "Variant.h"

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/VecFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/Conversion.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/StringHelpers.h>
#include <liboscar/Variant/VariantType.h>

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
    bool parse_as_bool(std::string_view str)
    {
        if (str.empty()) {
            return false;
        }
        else if (is_equal_case_insensitive(str, "false")) {
            return false;
        }
        else if (is_equal_case_insensitive(str, "0")) {
            return false;
        }
        else {
            return true;
        }
    }

    float parse_as_float_or_zero(std::string_view str)
    {
        // HACK: temporarily using `std::strof` here, rather than `std::from_chars` (C++17),
        // because MacOS with 14.2 SDK doesn't support the latter (as of May 2025) for
        // floating-point values.

        const std::string s{str};
        size_t pos = 0;
        try {
            return std::stof(s, &pos);
        }
        catch (const std::exception&) {
            return 0.0f;
        }
    }

    int parse_as_int_or_zero(std::string_view str)
    {
        int rv{};
        auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), rv);
        return err == std::errc{} ? rv : 0;
    }
}

osc::Variant::Variant() : data_{std::monostate{}}
{
    static_assert(std::variant_size_v<decltype(data_)> == num_options<VariantType>());
}
osc::Variant::Variant(bool v) : data_{v} {}
osc::Variant::Variant(Color v) : data_{v} {}
osc::Variant::Variant(float v) : data_{v} {}
osc::Variant::Variant(int v) : data_{v} {}
osc::Variant::Variant(std::string v) : data_{std::move(v)} {}
osc::Variant::Variant(std::string_view v) : data_{std::string{v}} {}
osc::Variant::Variant(const StringName& v) : data_{v} {}
osc::Variant::Variant(Vec2 v) : data_{v} {}
osc::Variant::Variant(Vec3 v) : data_{v} {}

VariantType osc::Variant::type() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return VariantType::None; },
        [](const bool&)           { return VariantType::Bool; },
        [](const Color&)          { return VariantType::Color; },
        [](const float&)          { return VariantType::Float; },
        [](const int&)            { return VariantType::Int; },
        [](const std::string&)    { return VariantType::String; },
        [](const StringName&)     { return VariantType::StringName; },
        [](const Vec2&)           { return VariantType::Vec2; },
        [](const Vec3&)           { return VariantType::Vec3; },
    }, data_);
}

osc::Variant::operator bool() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return false; },
        [](const bool& v)         { return v; },
        [](const Color& v)        { return v.r != 0.0f; },
        [](const float& v)        { return v != 0.0f; },
        [](const int& v)          { return v != 0; },
        [](std::string_view s)    { return parse_as_bool(s); },
        [](const Vec2& v)         { return v.x != 0.0f; },
        [](const Vec3& v)         { return v.x != 0.0f; },
    }, data_);
}

osc::Variant::operator osc::Color() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return Color::black(); },
        [](const bool& v)         { return v ? Color::white() : Color::black(); },
        [](const Color& v)        { return v; },
        [](const float& v)        { return Color{v}; },
        [](const int& v)          { return v ? Color::white() : Color::black(); },
        [](std::string_view str)
        {
            const auto c = try_parse_html_color_string(str);
            return c ? *c : Color::black();
        },
        [](const Vec2& v)         { return Color{v.x, v.y, 0.0f}; },
        [](const Vec3& v)         { return Color{v}; },
    }, data_);
}

osc::Variant::operator float() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return 0.0f; },
        [](const bool& v)         { return v ? 1.0f : 0.0f; },
        [](const Color& v)        { return v.r; },
        [](const float& v)        { return v; },
        [](const int& v)          { return static_cast<float>(v); },
        [](std::string_view s)    { return parse_as_float_or_zero(s); },
        [](const Vec2& v)         { return v.x; },
        [](const Vec3& v)         { return v.x; },
    }, data_);
}

osc::Variant::operator int() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return 0; },
        [](const bool& v)         { return v ? 1 : 0; },
        [](const Color& v)        { return static_cast<int>(v.r); },
        [](const float& v)        { return static_cast<int>(v); },
        [](const int& v)          { return v; },
        [](std::string_view s)    { return parse_as_int_or_zero(s); },
        [](const Vec2& v)         { return static_cast<int>(v.x); },
        [](const Vec3& v)         { return static_cast<int>(v.x); },
    }, data_);
}

osc::Variant::operator std::string() const
{
    using namespace std::literals::string_literals;

    return std::visit(Overload{
        [](const std::monostate&) { return "<null>"s; },
        [](const bool& v)         { return v ? "true"s : "false"s; },
        [](const Color& v)        { return to_html_string_rgba(v); },
        [](const float& v)        { return std::to_string(v); },
        [](const int& v)          { return std::to_string(v); },
        [](std::string_view s)    { return std::string{s}; },
        [](const Vec2& v)         { return stream_to_string(v); },
        [](const Vec3& v)         { return stream_to_string(v); },
    }, data_);
}

osc::Variant::operator osc::StringName() const
{
    return std::visit(Overload{
        [](const std::string& str) { return StringName{str}; },
        [](const StringName& sn)   { return sn; },
        [](const auto&)            { return StringName{}; },
    }, data_);
}

osc::Variant::operator osc::Vec2() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return Vec2{}; },
        [](const bool& v)         { return v ? Vec2{1.0f, 1.0f} : Vec2{}; },
        [](const Color& v)        { return Vec2{v.r, v.g}; },
        [](const float& v)        { return Vec2{v}; },
        [](const int& v)          { return Vec2{static_cast<float>(v)}; },
        [](std::string_view)      { return Vec2{}; },
        [](const Vec2& v)         { return v; },
        [](const Vec3& v)         { return Vec2{v}; },
    }, data_);
}

osc::Variant::operator osc::Vec3() const
{
    return std::visit(Overload{
        [](const std::monostate&) { return Vec3{}; },
        [](const bool& v)         { return v ? Vec3{1.0f, 1.0f, 1.0f} : Vec3{}; },
        [](const Color& v)        { return Vec3{v.r, v.g, v.b}; },
        [](const float& v)        { return Vec3{v}; },
        [](const int& v)          { return Vec3{static_cast<float>(v)}; },
        [](std::string_view)      { return Vec3{}; },
        [](const Vec2& v)         { return Vec3{v, 0.0f}; },
        [](const Vec3& v)         { return v; },
    }, data_);
}

bool osc::operator==(const Variant& lhs, const Variant& rhs)
{
    if (lhs.data_ == rhs.data_) {
        return true;   // same type, same value
    }

    if (lhs.data_.index() == rhs.data_.index()) {
        return false;  // same type, different value
    }

    if (std::holds_alternative<StringName>(lhs.data_) and std::holds_alternative<std::string>(rhs.data_)) {
        // edge-case: different type, but interconvertible to same value
        return std::get<StringName>(lhs.data_) == std::get<std::string>(rhs.data_);
    }

    if (std::holds_alternative<std::string>(lhs.data_) and std::holds_alternative<StringName>(rhs.data_)) {
        // edge-case: different type, but interconvertible to same value
        return std::get<std::string>(lhs.data_) == std::get<StringName>(rhs.data_);
    }

    return false;  // different type and non-interconvertible
}

std::ostream& osc::operator<<(std::ostream& out, const Variant& variant)
{
    return out << to<std::string>(variant);
}

size_t std::hash<osc::Variant>::operator()(const Variant& variant) const
{
    // note: you might be wondering why this isn't `std::hash<std::variant>{}(v.data_)`
    //
    // > cppreference.com
    // >
    // > Unlike `std::hash<std::optional>`, the hash of a `std::variant` does not typically equal the
    // > hash of the contained value; this makes it possible to distinguish `std::variant<int, int>`
    // > holding the same value as different alternatives.
    //
    // but `osc::Variant` doesn't need to support this edge-case, and transparent hashing of
    // the contents can be handy when callers want behavior like:
    //
    //     `hash_of(Variant) == hash_of(std::string) == hash_of(std::string_view) == hash_of(StringName)...`

    return std::visit(Overload{
        []<typename T>(const T& inner)
        {
            return std::hash<std::remove_cv_t<std::remove_reference_t<T>>>{}(inner);
        },
    }, variant.data_);
}
