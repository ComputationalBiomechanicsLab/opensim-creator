#pragma once

#include <oscar/Graphics/Color.h>
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
#include <initializer_list>
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

namespace osc
{
    class Mesh {
    public:
        Mesh();

        // affects how the backend renderer ultimately renders this mesh data
        // (e.g. would tell the OpenGL backend to draw with GL_LINES vs. GL_TRIANGLES)
        MeshTopology topology() const;
        void set_topology(MeshTopology);

        // vertex data: reassigning this causes attributes (normals, texture
        //              coordinates, colors, and tangents) to be resized
        bool has_vertices() const;
        size_t num_vertices() const;
        std::vector<Vec3> vertices() const;
        void set_vertices(std::span<const Vec3>);
        void set_vertices(std::initializer_list<Vec3> il)
        {
            set_vertices(std::span<const Vec3>{il});
        }
        void transform_vertices(const std::function<Vec3(Vec3)>&);
        void transform_vertices(const Transform&);
        void transform_vertices(const Mat4&);

        // attribute: you can only set an equal amount of normals to the number of
        //            vertices (or zero, which means "clear them")
        bool has_normals() const;
        std::vector<Vec3> normals() const;
        void set_normals(std::span<const Vec3>);
        void set_normals(std::initializer_list<Vec3> il)
        {
            set_normals(std::span<const Vec3>{il});
        }
        void transform_normals(const std::function<Vec3(Vec3)>&);

        // attribute: you can only set an equal amount of texture coordinates to
        //            the number of vertices (or zero, which means "clear them")
        bool has_tex_coords() const;
        std::vector<Vec2> tex_coords() const;
        void set_tex_coords(std::span<const Vec2>);
        void set_tex_coords(std::initializer_list<Vec2> il)
        {
            set_tex_coords(std::span<const Vec2>{il});
        }
        void transform_tex_coords(const std::function<Vec2(Vec2)>&);

        // attribute: you can only set an equal amount of colors to the number of
        //            vertices (or zero, which means "clear them")
        std::vector<Color> colors() const;
        void set_colors(std::span<const Color>);
        void set_colors(std::initializer_list<Color> il)
        {
            set_colors(std::span<const Color>{il});
        }

        // attribute: you can only set an equal amount of tangents to the number of
        //            vertices (or zero, which means "clear them")
        std::vector<Vec4> tangents() const;
        void set_tangents(std::span<const Vec4>);
        void set_tangents(std::initializer_list<Vec4> il)
        {
            set_tangents(std::span<const Vec4>{il});
        }

        // indices into the vertex data: tells the backend which primatives
        // to draw in which order from the underlying vertex buffer
        //
        // all meshes _must_ be indexed: even if you're just drawing a single triangle
        size_t num_indices() const;
        MeshIndicesView indices() const;
        void set_indices(MeshIndicesView, MeshUpdateFlags = MeshUpdateFlag::Default);
        void set_indices(std::initializer_list<uint32_t> il)
        {
            set_indices(MeshIndicesView{il});
        }
        void for_each_indexed_vertex(const std::function<void(Vec3)>&) const;
        void for_each_indexed_triangle(const std::function<void(Triangle)>&) const;
        Triangle get_triangle_at(size_t first_index_offset) const;
        std::vector<Vec3> indexed_vertices() const;

        // local-space bounds of the mesh
        //
        // automatically recalculated from the indexed data whenever `set_vertices`,
        // `set_indices`, or `set_vertex_buffer_data` is called
        const AABB& bounds() const;

        // clear all data in the mesh, such that the mesh then behaves as-if it were
        // just default-initialized
        void clear();

        // advanced api: submeshes
        //
        // enables describing sub-parts of the vertex buffer as independently-renderable
        // meshes. This is handy if (e.g.) you want to upload all of your mesh data
        // in one shot, or if you want to apply different materials to different parts
        // of the mesh, without having to create a bunch of separate vertex buffers
        size_t num_submesh_descriptors() const;
        void push_submesh_descriptor(const SubMeshDescriptor&);
        const SubMeshDescriptor& submesh_descriptor_at(size_t) const;
        template<std::ranges::input_range Range>
        requires std::constructible_from<SubMeshDescriptor, std::ranges::range_value_t<Range>>
        void set_submesh_descriptors(const Range& submesh_descriptors)
        {
            clear_submesh_descriptors();
            for (const auto& submesh_descriptor : submesh_descriptors) {
                push_submesh_descriptor(submesh_descriptor);
            }
        }
        void clear_submesh_descriptors();

        // advanced api: vertex attribute querying/layout/reformatting
        //
        // enables working with the actual layout of data on the CPU/GPU, so that
        // callers can (e.g.) upload all of their vertex data in one shot (rather than
        // calling each of the 'basic' methods above one-by-one, etc.)
        size_t num_vertex_attributes() const;
        const VertexFormat& vertex_format() const;
        void set_vertex_buffer_params(size_t num_vertices, const VertexFormat&);
        size_t vertex_buffer_stride() const;
        void set_vertex_buffer_data(std::span<const uint8_t>, MeshUpdateFlags = MeshUpdateFlag::Default);
        template<std::ranges::contiguous_range Range>
        requires BitCastable<typename Range::value_type>
        void set_vertex_buffer_data(const Range& range, MeshUpdateFlags flags = MeshUpdateFlag::Default)
        {
            std::span<const uint8_t> bytes = view_object_representations<uint8_t>(range);
            set_vertex_buffer_data(bytes, flags);
        }

        // recalculates the normals of the mesh from its triangles
        //
        // - does nothing if the mesh's topology is != MeshTopology::Triangles
        // - the normals of shared vertices are averaged (i.e. smooth-shaded)
        // - creates a normal vertex attribute if tangents aren't assigned yet
        void recalculate_normals();

        // recalculates the tangents of the mesh from its triangles + normals + texture coordinates
        //
        // - does nothing if the mesh's topology != MeshTopology::Triangles
        // - does nothing if the mesh has no normals
        // - does nothing if the mesh has no texture coordinates
        // - creates a tangent vertex attribute if tangents aren't assigned yet
        void recalculate_tangents();

        friend bool operator==(const Mesh&, const Mesh&) = default;
        friend std::ostream& operator<<(std::ostream&, const Mesh&);
    private:
        friend class GraphicsBackend;
        friend struct std::hash<Mesh>;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Mesh&);
}

template<>
struct std::hash<osc::Mesh> final {
    size_t operator()(const osc::Mesh& mesh) const
    {
        return std::hash<osc::CopyOnUpdPtr<osc::Mesh::Impl>>{}(mesh.impl_);
    }
};
