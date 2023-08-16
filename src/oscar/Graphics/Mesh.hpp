#pragma once

#include "oscar/Graphics/MeshIndicesView.hpp"
#include "oscar/Graphics/MeshTopology.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Utils/CopyOnUpdPtr.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
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

        nonstd::span<glm::vec3 const> getVerts() const;
        void setVerts(nonstd::span<glm::vec3 const>);
        void transformVerts(std::function<void(nonstd::span<glm::vec3>)> const&);
        void transformVerts(Transform const&);

        nonstd::span<glm::vec3 const> getNormals() const;
        void setNormals(nonstd::span<glm::vec3 const>);
        void transformNormals(std::function<void(nonstd::span<glm::vec3>)> const&);

        nonstd::span<glm::vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<glm::vec2 const>);
        void transformTexCoords(std::function<void(nonstd::span<glm::vec2>)> const&);

        nonstd::span<Color const> getColors() const;
        void setColors(nonstd::span<Color const>);

        nonstd::span<glm::vec4 const> getTangents() const;
        void setTangents(nonstd::span<glm::vec4 const>);

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

    private:
        friend class GraphicsBackend;
        friend struct std::hash<Mesh>;
        friend bool operator==(Mesh const&, Mesh const&) noexcept;
        friend bool operator!=(Mesh const&, Mesh const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, Mesh const&);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    inline bool operator==(Mesh const& a, Mesh const& b) noexcept
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(Mesh const& a, Mesh const& b) noexcept
    {
        return a.m_Impl != b.m_Impl;
    }

    std::ostream& operator<<(std::ostream&, Mesh const&);
}

namespace std
{
    template<>
    struct hash<osc::Mesh> final {
        size_t operator()(osc::Mesh const& mesh) const
        {
            using std::hash;
            return hash<osc::CopyOnUpdPtr<osc::Mesh::Impl>>{}(mesh.m_Impl);
        }
    };
}
