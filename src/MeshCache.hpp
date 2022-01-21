#pragma once

#include "src/3D/Mesh.hpp"

#include <memory>
#include <string>

namespace osc
{
    class MeshCache final {

    public:
        MeshCache();
        MeshCache(MeshCache const&) = delete;
        MeshCache(MeshCache&&) noexcept;
        ~MeshCache() noexcept;

        MeshCache& operator=(MeshCache const&) = delete;
        MeshCache& operator=(MeshCache&&) noexcept;

        // prints error to log and returns dummy mesh if load error happens
        std::shared_ptr<Mesh> getMeshFile(std::string const&);

        std::shared_ptr<Mesh> getSphereMesh();
        std::shared_ptr<Mesh> getCylinderMesh();
        std::shared_ptr<Mesh> getBrickMesh();
        std::shared_ptr<Mesh> getConeMesh();
        std::shared_ptr<Mesh> getFloorMesh();
        std::shared_ptr<Mesh> get100x100GridMesh();
        std::shared_ptr<Mesh> getCubeWireMesh();
        std::shared_ptr<Mesh> getYLineMesh();
        std::shared_ptr<Mesh> getTexturedQuadMesh();

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
