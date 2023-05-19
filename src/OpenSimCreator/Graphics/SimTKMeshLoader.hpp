#pragma once

#include <oscar/Graphics/Mesh.hpp>

#include <filesystem>

namespace SimTK { class PolygonalMesh; }

namespace osc
{
    Mesh ToOscMesh(SimTK::PolygonalMesh const&);
    Mesh LoadMeshViaSimTK(std::filesystem::path const&);
}