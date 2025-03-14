#include "MeshLoader.h"

#include <libopensimcreator/Graphics/SimTKMeshLoader.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>

#include <exception>
#include <filesystem>
#include <utility>
#include <vector>

osc::mi::MeshLoadResponse osc::mi::respondToMeshloadRequest(MeshLoadRequest msg)  // NOLINT(performance-unnecessary-value-param)
{
    std::vector<LoadedMesh> loadedMeshes;
    loadedMeshes.reserve(msg.paths.size());

    for (const std::filesystem::path& path : msg.paths)
    {
        try
        {
            loadedMeshes.push_back(LoadedMesh{path, LoadMeshViaSimTK(path)});
        }
        catch (const std::exception& ex)
        {
            // swallow the exception and emit a log error
            //
            // older implementations used to cancel loading the entire batch by returning
            // an MeshLoadErrorResponse, but that wasn't a good idea because there are
            // times when a user will drag in a bunch of files and expect all the valid
            // ones to load (#303)

            log_error("%s: error loading mesh file: %s", path.string().c_str(), ex.what());
        }
    }

    // ensure the UI thread redraws after the mesh is loaded
    App::upd().request_redraw();

    return MeshLoadOKResponse{msg.preferredAttachmentPoint, std::move(loadedMeshes)};
}
