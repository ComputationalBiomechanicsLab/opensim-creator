#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <cstddef>

namespace osc
{
    struct Triangle final {

        Vec3 const* data() const
        {
            return &p0;
        }

        Vec3 const* begin() const
        {
            return &p0;
        }

        Vec3 const* end() const
        {
            return &p2 + 1;
        }

        Vec3 const& operator[](size_t i) const
        {
            static_assert(sizeof(Triangle) == 3*sizeof(Vec3));
            static_assert(offsetof(Triangle, p0) == 0);
            return (&p0)[i];
        }

        friend bool operator==(Triangle const&, Triangle const&) = default;

        Vec3 p0{};
        Vec3 p1{};
        Vec3 p2{};
    };
}
