#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/StringName.h>
#include <liboscar/Variant/VariantType.h>

#include <cstddef>
#include <iosfwd>
#include <string_view>
#include <string>
#include <utility>
#include <variant>

namespace osc
{
    class Variant final {
    public:
        Variant();
        Variant(bool);
        Variant(Color);
        Variant(float);
        Variant(int);
        Variant(std::string);
        Variant(std::string_view);
        Variant(const char* ptr) : Variant{std::string_view{ptr}} {}
        Variant(std::nullopt_t) = delete;
        Variant(CStringView cstring_view) : Variant{std::string_view{cstring_view}} {}
        Variant(const StringName&);
        Variant(Vec2);
        Variant(Vec3);

        VariantType type() const;

        // implicit conversions
        explicit operator bool() const;
        operator Color() const;
        operator float() const;
        operator int() const;
        operator std::string() const;
        operator StringName() const;
        operator Vec2() const;
        operator Vec3() const;

        friend bool operator==(const Variant&, const Variant&);

        friend void swap(Variant& a, Variant& b) noexcept
        {
            std::swap(a.data_, b.data_);
        }

    private:
        friend struct std::hash<Variant>;

        std::variant<
            std::monostate,
            bool,
            Color,
            float,
            int,
            std::string,
            StringName,
            Vec2,
            Vec3
        > data_;
    };

    bool operator==(const Variant&, const Variant&);
    std::ostream& operator<<(std::ostream&, const Variant&);
}

template<>
struct std::hash<osc::Variant> final {
    size_t operator()(const osc::Variant&) const;
};
