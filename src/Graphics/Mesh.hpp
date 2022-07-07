#pragma once

#include "src/Graphics/MeshTopography.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Utils/CStringView.hpp"

#include <GL/glew.h>  // for GLenum
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gl
{
    class VertexArray;
}

namespace osc
{
    struct BVH;
    struct Line;
    struct MeshData;
    struct Transform;
}

namespace osc
{
    enum class IndexFormat {
        UInt16,
        UInt32,
    };

    class Mesh {
    public:
        Mesh(MeshData);
        Mesh(Mesh const&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh const&) = delete;
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh() noexcept;

        CStringView getName() const;
        void setName(std::string_view);

        MeshTopography getTopography() const;
        GLenum getTopographyOpenGL() const;
        void setTopography(MeshTopography);

        nonstd::span<glm::vec3 const> getVerts() const;
        void setVerts(nonstd::span<glm::vec3 const>);

        nonstd::span<glm::vec3 const> getNormals() const;
        void setNormals(nonstd::span<glm::vec3 const>);

        nonstd::span<glm::vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<glm::vec2 const>);
        void scaleTexCoords(float);

        IndexFormat getIndexFormat() const;
        GLenum getIndexFormatOpenGL() const;
        void setIndexFormat(IndexFormat);

        int getNumIndices() const;
        std::vector<uint32_t> getIndices() const;  // note: copies them, because IndexFormat may be U16 internally
        void setIndicesU16(nonstd::span<uint16_t const>);
        void setIndicesU32(nonstd::span<uint32_t const>);  // note: format trumps this, value will be truncated

        AABB const& getAABB() const;
        AABB getWorldspaceAABB(Transform const& localToWorldXform) const;
        AABB getWorldspaceAABB(glm::mat4x3 const& modelMatrix) const;

        BVH const& getTriangleBVH() const;

        // returns !hit if the line doesn't intersect it *or* the topography is not triangular
        RayCollision getClosestRayTriangleCollisionModelspace(Line const& modelspaceLine) const;

        // as above, but works in worldspace (requires a model matrix to map the worldspace line into modelspace)
        RayCollision getRayMeshCollisionInWorldspace(glm::mat4 const& model2world, Line const& worldspaceLine) const;

        void clear();
        void recalculateBounds();
        void uploadToGPU();  // must be called from GPU thread

        gl::VertexArray& GetVertexArray();
        void Draw();
        void DrawInstanced(size_t n);

    private:
        class Impl;
        Impl* m_Impl;
    };
}
