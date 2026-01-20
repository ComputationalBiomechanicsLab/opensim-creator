#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/mesh_indices_view.h>
#include <liboscar/platform/file_dialog_filter.h>

#include <filesystem>
#include <span>
#include <string_view>

namespace SimTK { class PolygonalMesh; }

namespace osc
{
    // returns an `Mesh` converted from the given `SimTK::PolygonalMesh`
    Mesh ToOscMesh(const SimTK::PolygonalMesh&);

    // returns a list of SimTK mesh format file suffixes (e.g. `{"vtp", "stl"}`)
    std::span<const std::string_view> GetSupportedSimTKMeshFormats();
    std::span<const FileDialogFilter> GetSupportedSimTKMeshFormatsAsFilters();

    // returns an `Mesh` loaded from disk via simbody's APIs
    Mesh LoadMeshViaSimbody(const std::filesystem::path&);

    // populate the `SimTK::PolygonalMesh` from the given indexed mesh data
    void AssignIndexedVerts(SimTK::PolygonalMesh&, std::span<const Vector3>, MeshIndicesView);
}
