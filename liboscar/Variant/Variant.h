#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
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
        Variant(const Variant&);
        Variant(Variant&&) noexcept;
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
        Variant(Vector2);
        Variant(Vector3);

        ~Variant() noexcept;

        Variant& operator=(const Variant&);
        Variant& operator=(Variant&&) noexcept;

        VariantType type() const;

        // implicit conversions
        explicit operator bool() const;
        operator Color() const;
        operator float() const;
        operator int() const;
        operator std::string() const;
        operator StringName() const;
        operator Vector2() const;
        operator Vector3() const;

        friend bool operator==(const Variant&, const Variant&);
        friend void swap(Variant&, Variant&) noexcept;

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
            Vector2,
            Vector3
        > data_;
    };

    bool operator==(const Variant&, const Variant&);
    void swap(Variant&, Variant&) noexcept;
    std::ostream& operator<<(std::ostream&, const Variant&);
}

template<>
struct std::hash<osc::Variant> final {
    size_t operator()(const osc::Variant&) const;
};
