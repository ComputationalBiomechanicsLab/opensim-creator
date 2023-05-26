#pragma once

#include <oscar/Graphics/Mesh.hpp>

#include <filesystem>
#include <string>

namespace SimTK { class PolygonalMesh; }

namespace osc
{
    Mesh ToOscMesh(SimTK::PolygonalMesh const&);
    std::string GetCommaDelimitedListOfSupportedSimTKMeshFormats();
    Mesh LoadMeshViaSimTK(std::filesystem::path const&);
}