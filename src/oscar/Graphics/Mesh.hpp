#pragma once

#include <oscar/Graphics/MeshIndicesView.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>

#include <nonstd/span.hpp>

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <vector>

namespace osc { struct AABB; }
namespace osc { class BVH; }
namespace osc { struct Color; }
namespace osc { struct Transform; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // mesh
    //
    // encapsulates mesh data, which may include vertices, indices, normals, texture
    // coordinates, etc.
    class Mesh final {
    public:
        Mesh();
        Mesh(Mesh const&);
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh const&);
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh() noexcept;

        MeshTopology getTopology() const;
        void setTopology(MeshTopology);

        nonstd::span<Vec3 const> getVerts() const;
        void setVerts(nonstd::span<Vec3 const>);
        void transformVerts(std::function<void(nonstd::span<Vec3>)> const&);
        void transformVerts(Transform const&);
        void transformVerts(Mat4 const&);

        nonstd::span<Vec3 const> getNormals() const;
        void setNormals(nonstd::span<Vec3 const>);
        void transformNormals(std::function<void(nonstd::span<Vec3>)> const&);

        nonstd::span<Vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<Vec2 const>);
        void transformTexCoords(std::function<void(nonstd::span<Vec2>)> const&);

        nonstd::span<Color const> getColors() const;
        void setColors(nonstd::span<Color const>);

        nonstd::span<Vec4 const> getTangents() const;
        void setTangents(nonstd::span<Vec4 const>);

        MeshIndicesView getIndices() const;
        void setIndices(MeshIndicesView);
        void setIndices(nonstd::span<uint16_t const>);
        void setIndices(nonstd::span<uint32_t const>);

        AABB const& getBounds() const;  // local-space
        BVH const& getBVH() const;  // local-space

        void clear();

        friend void swap(Mesh& a, Mesh& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(Mesh const& lhs, Mesh const& rhs) noexcept
        {
            return lhs.m_Impl == rhs.m_Impl;
        }

        friend bool operator!=(Mesh const& lhs, Mesh const& rhs) noexcept
        {
            return lhs.m_Impl != rhs.m_Impl;
        }

        friend std::ostream& operator<<(std::ostream&, Mesh const&);
    private:
        friend class GraphicsBackend;
        friend struct std::hash<Mesh>;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, Mesh const&);
}

template<>
struct std::hash<osc::Mesh> final {
    size_t operator()(osc::Mesh const& mesh) const
    {
        return std::hash<osc::CopyOnUpdPtr<osc::Mesh::Impl>>{}(mesh.m_Impl);
    }
};
