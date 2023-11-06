#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/VariantType.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
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
        Variant(char const*);
        Variant(std::nullopt_t) = delete;
        Variant(CStringView);
        Variant(StringName const&);
        Variant(Vec3);

        VariantType getType() const;

        // implicit conversions
        operator bool() const;
        operator Color() const;
        operator float() const;
        operator int() const;
        operator std::string() const;
        operator StringName() const;
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
        friend struct std::hash<osc::Variant>;

        std::variant<
            std::monostate,
            bool,
            Color,
            float,
            int,
            std::string,
            StringName,
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
