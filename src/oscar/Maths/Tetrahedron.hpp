#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <array>

namespace osc
{
    struct Tetrahedron final {

        Vec3 const* begin() const
        {
            return verts.data();
        }

        Vec3 const* end() const
        {
            return verts.data() + verts.size();
        }

        Vec3 const& operator[](size_t i) const
        {
            return verts[i];
        }

        Vec3& operator[](size_t i)
        {
            return verts[i];
        }

        constexpr size_t size() const
        {
            return 4;
        }

        std::array<Vec3, 4> verts{};
    };

    float Volume(Tetrahedron const&);
    Vec3 Center(Tetrahedron const&);
}
