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
        Variant(char const* ptr) : Variant{std::string_view{ptr}} {}
        Variant(std::nullopt_t) = delete;
        Variant(CStringView csv) : Variant{std::string_view{csv}} {}
        Variant(StringName const&);
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

        friend bool operator==(Variant const&, Variant const&);

        friend void swap(Variant& a, Variant& b) noexcept
        {
            std::swap(a.m_Data, b.m_Data);
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
        > m_Data;
    };

    bool operator==(Variant const&, Variant const&);
    std::string to_string(Variant const&);
    std::ostream& operator<<(std::ostream&, Variant const&);
}

template<>
struct std::hash<osc::Variant> final {
    size_t operator()(osc::Variant const&) const;
};
