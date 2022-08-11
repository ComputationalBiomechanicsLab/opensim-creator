#pragma once

#include "src/Graphics/MeshTopography.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <vector>

namespace osc
{
    struct AABB;
    struct BVH;
    struct Rgba32;
}

// note: implementation is in `Renderer.cpp`
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

        nonstd::span<glm::vec3 const> getNormals() const;
        void setNormals(nonstd::span<glm::vec3 const>);

        nonstd::span<glm::vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<glm::vec2 const>);

        nonstd::span<Rgba32 const> getColors();
        void setColors(nonstd::span<Rgba32 const>);

        int getNumIndices() const;
        std::vector<uint32_t> getIndices() const;
        void setIndices(nonstd::span<uint16_t const>);
        void setIndices(nonstd::span<uint32_t const>);

        AABB const& getBounds() const;  // local-space
        glm::vec3 getMidpoint() const;  // local-space

        // local-space: this is a hack that's only here to make porting from
        // the legacy `osc::Mesh` API easier - later iterations *should*
        // seperately associate the BVH with the mesh (e.g. via a BVH cache)
        BVH const& getBVH() const;

        void clear();

    private:
        friend class GraphicsBackend;
        friend bool operator==(Mesh const&, Mesh const&);
        friend bool operator!=(Mesh const&, Mesh const&);
        friend bool operator<(Mesh const&, Mesh const&);
        friend std::ostream& operator<<(std::ostream&, Mesh const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Mesh const&, Mesh const&);
    bool operator!=(Mesh const&, Mesh const&);
    bool operator<(Mesh const&, Mesh const&);
    std::ostream& operator<<(std::ostream&, Mesh const&);
}
