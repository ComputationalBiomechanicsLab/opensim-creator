#pragma once

#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/MeshUpdateFlags.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/ObjectRepresentation.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

namespace osc { struct AABB; }
namespace osc { struct Color; }
namespace osc { class SubMeshDescriptor; }
namespace osc { struct Transform; }
namespace osc { class VertexFormat; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    class Mesh final {
    public:
        Mesh();

        // affects how the backend renderer ultimately renders this mesh data
        // (e.g. would tell the OpenGL backend to draw with GL_LINES vs. GL_TRIANGLES)
        MeshTopology getTopology() const;
        void setTopology(MeshTopology);

        // vertex data: reassigning this causes attributes (normals, texture
        //              coordinates, colors, and tangents) to be resized
        bool hasVerts() const;
        size_t getNumVerts() const;
        std::vector<Vec3> getVerts() const;
        void setVerts(std::span<Vec3 const>);
        void transformVerts(std::function<Vec3(Vec3)> const&);
        void transformVerts(Transform const&);
        void transformVerts(Mat4 const&);

        // attribute: you can only set an equal amount of normals to the number of
        //            vertices (or zero, which means "clear them")
        bool hasNormals() const;
        std::vector<Vec3> getNormals() const;
        void setNormals(std::span<Vec3 const>);
        void transformNormals(std::function<Vec3(Vec3)> const&);

        // attribute: you can only set an equal amount of texture coordinates to
        //            the number of vertices (or zero, which means "clear them")
        bool hasTexCoords() const;
        std::vector<Vec2> getTexCoords() const;
        void setTexCoords(std::span<Vec2 const>);
        void transformTexCoords(std::function<Vec2(Vec2)> const&);

        // attribute: you can only set an equal amount of colors to the number of
        //            vertices (or zero, which means "clear them")
        std::vector<Color> getColors() const;
        void setColors(std::span<Color const>);

        // attribute: you can only set an equal amount of tangents to the number of
        //            vertices (or zero, which means "clear them")
        std::vector<Vec4> getTangents() const;
        void setTangents(std::span<Vec4 const>);

        // indices into the vertex data: tells the backend which primatives
        // to draw in which order from the underlying vertex buffer
        //
        // all meshes _must_ be indexed: even if you're just drawing a single triangle
        size_t getNumIndices() const;
        MeshIndicesView getIndices() const;
        void setIndices(MeshIndicesView, MeshUpdateFlags = MeshUpdateFlags::Default);
        void forEachIndexedVert(std::function<void(Vec3)> const&) const;
        void forEachIndexedTriangle(std::function<void(Triangle)> const&) const;
        Triangle getTriangleAt(size_t firstIndexOffset) const;
        std::vector<Vec3> getIndexedVerts() const;

        // local-space bounds of the mesh
        //
        // automatically recalculated from the indexed data whenever `setVerts`,
        // `setIndices`, or `setVertexBufferData` is called
        AABB const& getBounds() const;

        // clear all data in the mesh, such that the mesh then behaves as-if it were
        // just default-initialized
        void clear();

        // advanced api: submeshes
        //
        // enables describing sub-parts of the vertex buffer as independently-renderable
        // meshes. This is handy if (e.g.) you want to upload all of your mesh data
        // in one shot, or if you want to apply different materials to different parts
        // of the mesh, without having to create a bunch of separate vertex buffers
        size_t getSubMeshCount() const;
        void pushSubMeshDescriptor(SubMeshDescriptor const&);
        SubMeshDescriptor const& getSubMeshDescriptor(size_t) const;
        void clearSubMeshDescriptors();

        // advanced api: vertex attribute querying/layout/reformatting
        //
        // enables working with the actual layout of data on the CPU/GPU, so that
        // callers can (e.g.) upload all of their vertex data in one shot (rather than
        // calling each of the 'basic' methods above one-by-one, etc.)
        size_t getVertexAttributeCount() const;
        VertexFormat const& getVertexAttributes() const;
        void setVertexBufferParams(size_t n, VertexFormat const&);
        size_t getVertexBufferStride() const;
        void setVertexBufferData(std::span<uint8_t const>, MeshUpdateFlags = MeshUpdateFlags::Default);

        template<std::ranges::contiguous_range Range>
        void setVertexBufferData(Range const& range, MeshUpdateFlags flags = MeshUpdateFlags::Default)
            requires BitCastable<typename Range::value_type>
        {
            std::span<uint8_t const> bytes = ViewObjectRepresentations<uint8_t>(range);
            setVertexBufferData(bytes, flags);
        }

        friend void swap(Mesh& a, Mesh& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(Mesh const&, Mesh const&) = default;
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
