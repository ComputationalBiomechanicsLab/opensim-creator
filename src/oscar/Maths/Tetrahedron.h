#pragma once

#include <oscar/Maths/Vec3.h>

#include <array>
#include <cstddef>

namespace osc
{
    struct Tetrahedron final {
        using value_type = Vec3;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = Vec3&;
        using const_reference = Vec3 const&;
        using pointer = Vec3*;
        using const_pointer = Vec3 const*;
        using iterator = Vec3*;
        using const_iterator = Vec3 const*;

        constexpr size_t size() const { return 4; }
        constexpr pointer data() { return &p0; }
        constexpr const_pointer data() const { return &p0; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_t i) { return begin()[i]; }
        constexpr const_reference operator[](size_t i) const { return begin()[i]; }

        value_type p0{};
        value_type p1{};
        value_type p2{};
        value_type p3{};
    };
}
