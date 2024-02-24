#pragma once

#include <oscar/Maths/Vec3.h>

#include <cstddef>

namespace osc
{
    struct Triangle final {

        using T = float;

        constexpr size_t size() const
        {
            return 3;
        }

        Vec<3, T> const* data() const
        {
            return &p0;
        }

        Vec<3, T> const* begin() const
        {
            return &p0;
        }

        Vec<3, T> const* end() const
        {
            return &p2 + 1;
        }

        Vec<3, T> const& operator[](size_t i) const
        {
            static_assert(sizeof(Triangle) == 3*sizeof(Vec<3, T>));
            static_assert(offsetof(Triangle, p0) == 0);
            return (&p0)[i];
        }

        friend bool operator==(Triangle const&, Triangle const&) = default;

        Vec<3, T> p0{};
        Vec<3, T> p1{};
        Vec<3, T> p2{};
    };
}
