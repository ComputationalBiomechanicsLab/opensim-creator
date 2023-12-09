#pragma once

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Utils/Spsc.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// background mesh loading support
//
// loading mesh files can be slow, so all mesh loading is done on a background worker
// that:
//
//   - receives a mesh loading request
//   - loads the mesh
//   - sends the loaded mesh (or error) as a response
//
// the main (UI) thread then regularly polls the response channel and handles the (loaded)
// mesh appropriately
namespace osc::mi
{
    // a mesh loading request
    struct MeshLoadRequest final {
        UID preferredAttachmentPoint;
        std::vector<std::filesystem::path> paths;
    };

    // a successfully-loaded mesh
    struct LoadedMesh final {
        std::filesystem::path path;
        osc::Mesh meshData;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        UID preferredAttachmentPoint;
        std::vector<LoadedMesh> meshes;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        UID preferredAttachmentPoint;
        std::filesystem::path path;
        std::string error;
    };

    // an OK or ERROR response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // returns an OK or ERROR response to a mesh load request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest);

    // a class that loads meshes in a background thread
    //
    // the UI thread must `.poll()` this to check for responses
    class MeshLoader final {
    public:
        MeshLoader() : m_Worker{Worker::create(respondToMeshloadRequest)}
        {
        }

        void send(MeshLoadRequest req)
        {
            m_Worker.send(std::move(req));
        }

        std::optional<MeshLoadResponse> poll()
        {
            return m_Worker.poll();
        }

    private:
        using Worker = spsc::Worker<
            MeshLoadRequest,
            MeshLoadResponse,
            decltype(respondToMeshloadRequest)
        >;
        Worker m_Worker;
    };
}
