#pragma once

#include "src/3D/Model.hpp"
#include "src/3D/BVH.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <vector>
#include <cstdint>

namespace osc {
    class ImmutableSceneMesh final {
    public:
        using IdType = int64_t;

        ImmutableSceneMesh(Mesh const&);

        IdType getID() const noexcept;  // globally unique
        Mesh const& getMesh() const noexcept;
        nonstd::span<glm::vec3 const> getVerts() const noexcept;
        nonstd::span<glm::vec3 const> getNormals() const noexcept;
        nonstd::span<glm::vec2 const> getTexCoords() const noexcept;
        std::vector<unsigned short> const& getIndices() const noexcept;
        AABB const& getAABB() const noexcept;
        Sphere const& getBoundingSphere() const noexcept;
        BVH const& getTriangleBVH() const noexcept;

    private:
        IdType m_ID;
        Mesh m_Mesh;
        AABB m_AABB;
        Sphere m_BoundingSphere;
        BVH m_TriangleBVH;
    };
}
