#pragma once

#include <oscar/Maths/Vec3.h>

#include <cstddef>

namespace osc
{
    struct Triangle final {
        using element_type = float;
        using value_type = Vec<3, element_type>;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        constexpr size_type size() const { return 3; }
        constexpr pointer data() { return &p0; }
        constexpr const_pointer data() const { return &p0; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type i) { return begin()[i]; }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        friend bool operator==(const Triangle&, const Triangle&) = default;

        value_type p0{};
        value_type p1{};
        value_type p2{};
    };
}
