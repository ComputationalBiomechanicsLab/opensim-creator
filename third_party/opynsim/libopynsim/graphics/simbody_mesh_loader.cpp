#include "simbody_mesh_loader.h"

#include <libopynsim/utilities/simbody_x_oscar.h>

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/triangle_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/file_dialog_filter.h>
#include <liboscar/utilities/assertions.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <vector>

using namespace opyn;
using namespace std::literals;
namespace rgs = std::ranges;

namespace
{
    constexpr auto c_supported_mesh_extensions = std::to_array({"obj"sv, "vtp"sv, "stl"sv, "stla"sv});

    std::span<const osc::FileDialogFilter> get_file_dialog_filters()
    {
        static const auto s_filters = std::to_array<osc::FileDialogFilter>({
            osc::FileDialogFilter{"Mesh Data (*.obj, *.vtp, *.stl, *.stla)", "obj;vtp;stl;stla"},
            osc::FileDialogFilter{"Wavefront (*.obj)", "obj"},
            osc::FileDialogFilter{"VTK PolyData (*.vtp)", "vtp"},
            osc::FileDialogFilter{"STL (*.stl)", "stl"},
            osc::FileDialogFilter{"ASCII STL (*.stla)", "stla"},
            osc::FileDialogFilter::all_files(),
        });
        return s_filters;
    }

    struct OutputMeshMetrics {
        size_t numVertices = 0;
        size_t numIndices = 0;
    };

    OutputMeshMetrics calc_mesh_metrics(const SimTK::PolygonalMesh& mesh)
    {
        OutputMeshMetrics rv;
        rv.numVertices = mesh.getNumVertices();
        for (int i = 0, faces = mesh.getNumFaces(); i < faces; ++i) {
            const int num_face_verts = mesh.getNumVerticesForFace(i);
            if (num_face_verts < 3) {
                continue;  // ignore lines/points
            }
            else if (num_face_verts == 3) {
                rv.numIndices += 3;
            }
            else if (num_face_verts == 4) {
                rv.numIndices += 6;
            }
            else {
                rv.numVertices += 1;  // triangulation
                rv.numIndices += static_cast<size_t>(3*(num_face_verts-2));
            }
        }
        return rv;
    }
}

osc::Mesh opyn::to_osc_mesh(const SimTK::PolygonalMesh& mesh)
{
    const auto metrics = calc_mesh_metrics(mesh);

    std::vector<osc::Vector3> vertices;
    vertices.reserve(metrics.numVertices);

    std::vector<uint32_t> indices;
    indices.reserve(metrics.numIndices);

    // helper: validate+push triangle into the index list
    const auto push_triangle = [&indices, &vertices](uint32_t a, uint32_t b, uint32_t c)
    {
        if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size()) {
            return;  // index out-of-bounds
        }

        if (!can_form_triangle(vertices[a], vertices[b], vertices[c])) {
            return;  // vertex data doesn't form a triangle (NaNs, degenerate locations)
        }

        indices.insert(indices.end(), {a, b, c});
    };

    // copy all vertex positions from the source mesh
    for (int i = 0; i < mesh.getNumVertices(); ++i) {
        vertices.push_back(osc::to<osc::Vector3>(mesh.getVertexPosition(i)));
    }

    // build up the index list while triangulating any n>3 faces
    //
    // (pushes injected triangulation vertices to the end - assumes the mesh is optimized later)
    for (int face = 0, faces = mesh.getNumFaces(); face < faces; ++face) {
        const int num_face_verts = mesh.getNumVerticesForFace(face);

        if (num_face_verts <= 1) {
            // point (ignore)
            continue;
        }
        if (num_face_verts == 2) {
            // line (ignore)
            continue;
        }
        else if (num_face_verts == 3) {
            // triangle
            const auto a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));

            push_triangle(a, b, c);
        }
        else if (num_face_verts == 4) {
            // quad (emit as two triangles)
            const auto a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));
            const auto d = static_cast<uint32_t>(mesh.getFaceVertex(face, 3));

            push_triangle(a, b, c);
            push_triangle(a, c, d);
        }
        else {
            // polygon: triangulate each edge with a centroid

            // compute+add centroid vertex
            osc::Vector3 centroid_of{};
            for (int vert = 0; vert < num_face_verts; ++vert) {
                centroid_of += vertices.at(mesh.getFaceVertex(face, vert));
            }
            centroid_of /= static_cast<float>(num_face_verts);
            const auto centroid_idx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(centroid_of);

            // triangulate polygon loop
            for (int vert = 0; vert < num_face_verts-1; ++vert) {
                const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, vert));
                const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, vert+1));

                push_triangle(centroid_idx, b, c);
            }

            // (complete the loop)
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, num_face_verts-1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            push_triangle(centroid_idx, b, c);
        }
    }

    osc::Mesh rv;
    rv.set_vertices(vertices);
    rv.set_indices(indices);
    rv.recalculate_normals();
    return rv;
}

std::span<const std::string_view> opyn::get_supported_sim_tk_mesh_formats()
{
    return c_supported_mesh_extensions;
}

std::span<const osc::FileDialogFilter> opyn::get_supported_sim_tk_mesh_formats_as_filters()
{
    return get_file_dialog_filters();
}

osc::Mesh opyn::load_mesh_via_simbody(const std::filesystem::path& p)
{
    const SimTK::DecorativeMeshFile dmf{p.string()};
    const SimTK::PolygonalMesh& mesh = dmf.getMesh();
    return to_osc_mesh(mesh);
}

void opyn::assign_indexed_verts(SimTK::PolygonalMesh& mesh, std::span<const osc::Vector3> vertices, osc::MeshIndicesView indices)
{
    mesh.clear();

    // assign vertices
    for (const osc::Vector3& vertex : vertices) {
        mesh.addVertex(to<SimTK::Vec3>(vertex));
    }

    // assign indices (assumed triangle)
    OSC_ASSERT_ALWAYS(indices.size() % 3 == 0);
    SimTK::Array_<int> tri_verts(3, 0);
    for (auto it = rgs::begin(indices); it != rgs::end(indices); it += 3) {
        tri_verts[0] = static_cast<int>(it[0]);
        tri_verts[1] = static_cast<int>(it[1]);
        tri_verts[2] = static_cast<int>(it[2]);
        mesh.addFace(tri_verts);
    }
}
