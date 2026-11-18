#include "mesh_loader.h"

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>

#include <exception>
#include <filesystem>
#include <utility>
#include <vector>

osc::MeshLoadResponse osc::respond_to_meshload_request(MeshLoadRequest msg)
{
    std::vector<LoadedMesh> loaded_meshes;
    loaded_meshes.reserve(msg.paths.size());

    for (const std::filesystem::path& path : msg.paths) {
        try {
            loaded_meshes.push_back(LoadedMesh{path, opyn::load_mesh_via_simbody(path)});
        } catch (const std::exception& ex) {
            // swallow the exception and emit a log error
            //
            // older implementations used to cancel loading the entire batch by returning
            // an MeshLoadErrorResponse, but that wasn't a good idea because there are
            // times when a user will drag in a bunch of files and expect all the valid
            // ones to load (#303)

            log_error("{}: error loading mesh file: {}", path.string(), ex.what());
        }
    }

    // ensure the UI thread redraws after the mesh is loaded
    App::upd().request_redraw();

    return MeshLoadOKResponse{msg.preferred_attachment_point, std::move(loaded_meshes)};
}
