#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar_document/StringName.hpp>
#include <oscar_document/VariantType.hpp>

#include <glm/vec3.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace osc::doc
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
        Variant(glm::vec3);

        VariantType getType() const;

        // implicit conversions
        operator bool() const;
        operator Color() const;
        operator float() const;
        operator int() const;
        operator std::string() const;
        operator StringName() const;
        operator glm::vec3() const;

        // explicit conversion
        template<typename T>
        T to() const
        {
            return this->operator T();
        }

        friend bool operator==(Variant const&, Variant const&);
        friend bool operator!=(Variant const&, Variant const&);

        friend void swap(Variant& a, Variant& b) noexcept
        {
            std::swap(a, b);
        }

    private:
        friend struct std::hash<osc::doc::Variant>;

        std::variant<
            std::monostate,
            bool,
            Color,
            float,
            int,
            std::string,
            StringName,
            glm::vec3
        > m_Data;
    };

    bool operator==(Variant const&, Variant const&);
    bool operator!=(Variant const&, Variant const&);
    std::string to_string(Variant const&);
    std::ostream& operator<<(std::ostream&, Variant const&);
}

template<>
struct std::hash<osc::doc::Variant> final {
    size_t operator()(osc::doc::Variant const&) const;
};
