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
#include <variant>
#include <vector>

namespace osc
{
    class Variant final {
    public:
        Variant();
        Variant(const Variant&);
        Variant(Variant&&) noexcept;
        explicit Variant(bool);
        explicit Variant(Color);
        explicit Variant(float);
        explicit Variant(int);
        explicit Variant(std::string);
        explicit Variant(std::string_view);
        explicit Variant(const char* ptr) : Variant{std::string_view{ptr}} {}
        explicit Variant(std::nullopt_t) = delete;
        explicit Variant(CStringView cstring_view) : Variant{std::string_view{cstring_view}} {}
        explicit Variant(const StringName&);
        explicit Variant(Vector2);
        explicit Variant(Vector3);
        explicit Variant(std::vector<Variant>);

        ~Variant() noexcept;

        Variant& operator=(const Variant&);
        Variant& operator=(Variant&&) noexcept;

        VariantType type() const;

        // implicit conversions
        explicit operator bool() const;
        explicit operator Color() const;
        explicit operator float() const;
        explicit operator int() const;
        explicit operator std::string() const;
        explicit operator StringName() const;
        explicit operator Vector2() const;
        explicit operator Vector3() const;
        explicit operator std::vector<Variant>() const;

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
            Vector3,
            std::vector<Variant>
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
