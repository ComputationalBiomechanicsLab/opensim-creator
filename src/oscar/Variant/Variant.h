#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/VariantType.h>

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
        Variant(CStringView cstr) : Variant{std::string_view{cstr}} {}
        Variant(const StringName&);
        Variant(Vec2);
        Variant(Vec3);

        VariantType type() const;

        // implicit conversions
        operator bool() const;
        operator Color() const;
        operator float() const;
        operator int() const;
        operator std::string() const;
        operator StringName() const;
        operator Vec2() const;
        operator Vec3() const;

        // explicit conversion
        template<typename T>
        T to() const
        {
            return this->operator T();
        }

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
    std::string to_string(const Variant&);
    std::ostream& operator<<(std::ostream&, const Variant&);
}

template<>
struct std::hash<osc::Variant> final {
    size_t operator()(const osc::Variant&) const;
};
