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
        using const_reference = const Vec3&;
        using pointer = Vec3*;
        using const_pointer = const Vec3*;
        using iterator = Vec3*;
        using const_iterator = const Vec3*;

        constexpr size_t size() const { return 4; }
        constexpr pointer data() { return &p0; }
        constexpr const_pointer data() const { return &p0; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_t pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_t pos) const { return begin()[pos]; }

        value_type p0{};
        value_type p1{};
        value_type p2{};
        value_type p3{};
    };
}
