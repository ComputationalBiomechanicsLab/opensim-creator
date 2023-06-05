#pragma once

#include <oscar/Graphics/Mesh.hpp>

#include <filesystem>
#include <string>

namespace SimTK { class PolygonalMesh; }

namespace osc
{
    // returns an `osc::Mesh` converted from the given `SimTK::PolygonalMesh`
    Mesh ToOscMesh(SimTK::PolygonalMesh const&);

    // returns a comma-delimited list of SimTK mesh format file suffixes (e.g. `vtp,stl`)
    std::string GetCommaDelimitedListOfSupportedSimTKMeshFormats();

    // returns an `osc::Mesh` loaded from disk via SimTK's APIs
    Mesh LoadMeshViaSimTK(std::filesystem::path const&);
}