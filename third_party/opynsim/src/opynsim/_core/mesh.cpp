#include "mesh.h"

#include <opynsim/_core/utilities/nb_helpers.h>

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector.h>
#include <nanobind/nanobind.h>
#pragma warning(push)
#pragma warning(disable : 4702)  // Disable "unreachable code"
#include <nanobind/ndarray.h>
#pragma warning(pop)

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    nb::ndarray<nb::numpy, const float, nb::shape<-1, 3>> mesh_vertices_impl(const osc::Mesh& mesh)
    {
        static_assert(
            sizeof(osc::Vector3f) == 3 * sizeof(float),
            "required to cast the pointer to float with no gaps between vertices");
        static_assert(alignof(osc::Vector3f) == alignof(float));

        // Copy the vertices out of the `Mesh` and into a capsule-friendly pointer
        auto vertices = std::make_unique<std::vector<osc::Vector3f>>(mesh.vertices());

        // Cast the front vertex pointer from `const Vector3f*` to `const float*`, so that
        // it matches what the ndarray API needs.
        const auto* data_ptr = std::launder(reinterpret_cast<const float*>(vertices->data()));
        constexpr size_t ndim = 2;
        const auto shape = std::to_array<size_t>({vertices->size(), 3});

        return {data_ptr, ndim, shape.data(), to_capsule(std::move(vertices))};
    }

    void mesh_set_vertices_impl(
        osc::Mesh& mesh,
        const nb::ndarray<nb::ro, nb::shape<-1, 3>, nb::c_contig, nb::device::cpu>& input)
    {
        const size_t num_vertices = input.shape(0);

        if (input.dtype() == nb::dtype<float>()) {
            // Fast path: the vertex data from the caller matches the float32 format
            // that's always presented via the getters, so just copy their data into
            // the mesh.

            static_assert(sizeof(osc::Vector3f) == 3 * sizeof(float));
            static_assert(alignof(osc::Vector3f) <= alignof(float));
            mesh.set_vertices({static_cast<const osc::Vector3f*>(input.data()), num_vertices});

        } else if (input.dtype() == nb::dtype<double>()) {
            // Slow path: the vertex data from the caller is 64-bit, but the `Mesh` class
            // always returns 32-bit encoded vertices so, to keep things simple, the data
            // should be converted to 32-bit.

            static_assert(sizeof(osc::Vector3d) == 3 * sizeof(double));
            static_assert(alignof(osc::Vector3d) <= alignof(double));
            const std::span<const osc::Vector3d> source_vertices{
                static_cast<const osc::Vector3d*>(input.data()), num_vertices};

            std::vector<osc::Vector3f> converted_vertices;
            converted_vertices.reserve(num_vertices);
            for (const auto& source_vertex : source_vertices) {
                converted_vertices.emplace_back(source_vertex);
            }
            mesh.set_vertices(converted_vertices);
        } else {
            throw nb::type_error("Unsupported array type! Property accepts float32 or float64.");
        }
    }

    nb::ndarray<nb::numpy, const int32_t, nb::shape<-1>> mesh_faces_impl(const osc::Mesh& mesh)
    {
        // Read the `Mesh`'s indices into a pointer that can be put into a capsule
        const auto indices_view = mesh.indices();
        auto faces = std::make_unique<std::vector<int32_t>>();
        faces->reserve(indices_view.size());
        for (const auto& index : indices_view) {
            faces->emplace_back(index);
        }

        const int32_t* faces_ptr = faces->data();
        const size_t size = faces->size();

        return {faces_ptr, {size}, to_capsule(std::move(faces))};
    }

    template<typename To, typename From>
    void mesh_assign_converted_indices(osc::Mesh& mesh, std::span<const From> from)
    {
        std::vector<To> data;
        data.reserve(from.size());
        for (const From& face : from) {
            data.emplace_back(static_cast<To>(face));
        }
        mesh.set_indices(data);
    }

    void mesh_set_faces_impl(
        osc::Mesh& mesh,
        const nb::ndarray<nb::ro, nb::shape<-1>, nb::c_contig, nb::device::cpu>& input)
    {
        const size_t num_faces = input.shape(0);

        if (input.dtype() == nb::dtype<int32_t>()) {
            const std::span<const int32_t> faces{
                static_cast<const int32_t*>(input.data()), num_faces};
            mesh_assign_converted_indices<uint32_t>(mesh, faces);
        } else if (input.dtype() == nb::dtype<int64_t>()) {
            const std::span<const int64_t> faces{
                static_cast<const int64_t*>(input.data()), num_faces};
            mesh_assign_converted_indices<uint32_t>(mesh, faces);
        } else {
            throw nb::type_error(
                "Unsupported array type! The faces property only accepts int32/int64 data types.");
        }
    }
}

void opyn::python::def_mesh(nanobind::module_& m)
{
    nb::class_<osc::Mesh> mesh_class(
        m,
        "Mesh",
        R"(
            Represents a renderable mesh.
        )");
    mesh_class.def(nb::init<>());
    mesh_class.def_prop_rw(
        "vertices",
        mesh_vertices_impl,
        mesh_set_vertices_impl,
        nb::rv_policy::move,
        R"(
            Gets/sets the vertices of the mesh as an Nx3 ndarray.
        )");
    mesh_class.def_prop_rw(
        "faces",
        mesh_faces_impl,
        mesh_set_faces_impl,
        nb::rv_policy::move,
        R"(
            Gets/sets the faces (indices) of the mesh as an 1-dimensional ndarray.
        )");
}
