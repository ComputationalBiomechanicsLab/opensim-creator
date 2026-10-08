#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/mesh_indices_view.h>
#include <liboscar/platform/file_dialog_filter.h>

#include <filesystem>
#include <span>
#include <string_view>

namespace SimTK { class PolygonalMesh; }

namespace opyn
{
    // returns an `Mesh` converted from the given `SimTK::PolygonalMesh`
    osc::Mesh to_osc_mesh(const SimTK::PolygonalMesh&);

    // returns a list of SimTK mesh format file suffixes (e.g. `{"vtp", "stl"}`)
    std::span<const std::string_view> get_supported_sim_tk_mesh_formats();
    std::span<const osc::FileDialogFilter> get_supported_sim_tk_mesh_formats_as_filters();

    // returns an `Mesh` loaded from disk via simbody's APIs
    osc::Mesh load_mesh_via_simbody(const std::filesystem::path&);

    // populate the `SimTK::PolygonalMesh` from the given indexed mesh data
    void assign_indexed_verts(SimTK::PolygonalMesh&, std::span<const osc::Vector3>, osc::MeshIndicesView);
}
