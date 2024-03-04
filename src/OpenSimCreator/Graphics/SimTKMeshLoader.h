#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshIndicesView.h>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace SimTK { class PolygonalMesh; }

namespace osc
{
    // returns an `Mesh` converted from the given `SimTK::PolygonalMesh`
    Mesh ToOscMesh(SimTK::PolygonalMesh const&);

    // returns a comma-delimited list of SimTK mesh format file suffixes (e.g. `vtp,stl`)
    std::string GetCommaDelimitedListOfSupportedSimTKMeshFormats();

    // returns an `Mesh` loaded from disk via SimTK's APIs
    Mesh LoadMeshViaSimTK(std::filesystem::path const&);

    // populate the `SimTK::PolygonalMesh` from the given indexed mesh data
    void AssignIndexedVerts(SimTK::PolygonalMesh&, std::span<Vec3 const>, MeshIndicesView);
}
