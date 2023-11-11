#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshIndicesView.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <span>

namespace osc { struct AABB; }

namespace osc
{
    // a readonly, hittest-able, mesh
    class SceneMesh final {
    public:
        SceneMesh();
        explicit SceneMesh(Mesh const&);

        BVH const& getBVH() const
        {
            return m_Data->bvh;
        }

        Mesh const& getUnderlyingMesh() const
        {
            return m_Data->mesh;
        }

        operator Mesh const& () const
        {
            return m_Data->mesh;
        }

        MeshTopology getTopology() const
        {
            return getUnderlyingMesh().getTopology();
        }

        std::span<Vec3 const> getVerts() const
        {
            return getUnderlyingMesh().getVerts();
        }

        std::span<Vec3 const> getNormals() const
        {
            return getUnderlyingMesh().getNormals();
        }

        std::span<Vec2 const> getTexCoords() const
        {
            return getUnderlyingMesh().getTexCoords();
        }

        std::span<Color const> getColors() const
        {
            return getUnderlyingMesh().getColors();
        }

        std::span<Vec4 const> getTangents() const
        {
            return getUnderlyingMesh().getTangents();
        }

        MeshIndicesView getIndices() const
        {
            return getUnderlyingMesh().getIndices();
        }

        AABB const& getBounds() const
        {
            return getUnderlyingMesh().getBounds();
        }

        friend void swap(SceneMesh& a, SceneMesh& b) noexcept
        {
            swap(a.m_Data, b.m_Data);
        }

        friend bool operator==(SceneMesh const& lhs, SceneMesh const& rhs)
        {
            return lhs.getUnderlyingMesh() == rhs.getUnderlyingMesh();
        }

        friend std::ostream& operator<<(std::ostream& lhs, SceneMesh const& rhs)
        {
            return lhs << rhs.getUnderlyingMesh();
        }
    private:
        friend struct std::hash<SceneMesh>;

        struct Data final {
            Data() = default;
            Data(Mesh const& mesh_, BVH&& bvh_) : mesh{mesh_}, bvh{std::move(bvh_)} {}

            Mesh mesh;
            BVH bvh;
        };
        std::shared_ptr<Data const> m_Data;
    };
}

template<>
struct std::hash<osc::SceneMesh> final {
    size_t operator()(osc::SceneMesh const& sm) const
    {
        return std::hash<osc::Mesh>{}(sm.getUnderlyingMesh());
    }
};
