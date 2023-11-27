#include "MeshLoader.hpp"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>

#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>

#include <filesystem>
#include <exception>
#include <utility>
#include <vector>

osc::MeshLoadResponse osc::respondToMeshloadRequest(MeshLoadRequest msg)  // NOLINT(performance-unnecessary-value-param)
{
    std::vector<LoadedMesh> loadedMeshes;
    loadedMeshes.reserve(msg.paths.size());

    for (std::filesystem::path const& path : msg.paths)
    {
        try
        {
            loadedMeshes.push_back(LoadedMesh{path, osc::LoadMeshViaSimTK(path)});
        }
        catch (std::exception const& ex)
        {
            // swallow the exception and emit a log error
            //
            // older implementations used to cancel loading the entire batch by returning
            // an MeshLoadErrorResponse, but that wasn't a good idea because there are
            // times when a user will drag in a bunch of files and expect all the valid
            // ones to load (#303)

            osc::log::error("%s: error loading mesh file: %s", path.string().c_str(), ex.what());
        }
    }

    // ensure the UI thread redraws after the mesh is loaded
    App::upd().requestRedraw();

    return MeshLoadOKResponse{msg.preferredAttachmentPoint, std::move(loadedMeshes)};
}
