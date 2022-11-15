#pragma once

#include "src/Graphics/MeshIndicesView.hpp"
#include "src/Graphics/MeshTopography.hpp"
#include "src/Utils/Cow.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <vector>

namespace osc { struct AABB; }
namespace osc { struct BVH; }
namespace osc { struct Rgba32; }

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

        MeshTopography getTopography() const;
        void setTopography(MeshTopography);

        nonstd::span<glm::vec3 const> getVerts() const;
        void setVerts(nonstd::span<glm::vec3 const>);
        void transformVerts(std::function<void(nonstd::span<glm::vec3>)> const&);

        nonstd::span<glm::vec3 const> getNormals() const;
        void setNormals(nonstd::span<glm::vec3 const>);
        void transformNormals(std::function<void(nonstd::span<glm::vec3>)> const&);

        nonstd::span<glm::vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<glm::vec2 const>);

        nonstd::span<Rgba32 const> getColors() const;
        void setColors(nonstd::span<Rgba32 const>);

        MeshIndicesView getIndices() const;
        void setIndices(MeshIndicesView);
        void setIndices(nonstd::span<uint16_t const>);
        void setIndices(nonstd::span<uint32_t const>);

        AABB const& getBounds() const;  // local-space
        glm::vec3 getMidpoint() const;  // local-space

        // local-space: this is a hack that's only here to make porting from
        // the legacy `osc::Mesh` API easier - later iterations *should*
        // seperately associate the BVH with the mesh (e.g. via a BVH cache)
        BVH const& getBVH() const;

        void clear();

        friend void swap(Mesh& a, Mesh& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend class GraphicsBackend;
        friend bool operator==(Mesh const&, Mesh const&) noexcept;
        friend bool operator!=(Mesh const&, Mesh const&) noexcept;
        friend bool operator<(Mesh const&, Mesh const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, Mesh const&);

        class Impl;
        Cow<Impl> m_Impl;
    };

    inline bool operator==(Mesh const& a, Mesh const& b) noexcept
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(Mesh const& a, Mesh const& b) noexcept
    {
        return a.m_Impl != b.m_Impl;
    }

    inline bool operator<(Mesh const& a, Mesh const& b) noexcept
    {
        return a.m_Impl < b.m_Impl;
    }

    std::ostream& operator<<(std::ostream&, Mesh const&);
}
