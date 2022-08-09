#pragma once

#include <memory>
#include <string>

namespace osc::experimental
{
    class Mesh;
}

namespace osc
{
    class MeshCache final {

    public:
        MeshCache();
        MeshCache(MeshCache const&) = delete;
        MeshCache(MeshCache&&) noexcept;
        MeshCache& operator=(MeshCache const&) = delete;
        MeshCache& operator=(MeshCache&&) noexcept;
        ~MeshCache() noexcept;

        // prints error to log and returns dummy mesh if load error happens
        std::shared_ptr<experimental::Mesh const> getMeshFile(std::string const&);

        std::shared_ptr<experimental::Mesh const> getSphereMesh();
        std::shared_ptr<experimental::Mesh const> getCylinderMesh();
        std::shared_ptr<experimental::Mesh const> getBrickMesh();
        std::shared_ptr<experimental::Mesh const> getConeMesh();
        std::shared_ptr<experimental::Mesh const> getFloorMesh();
        std::shared_ptr<experimental::Mesh const> get100x100GridMesh();
        std::shared_ptr<experimental::Mesh const> getCubeWireMesh();
        std::shared_ptr<experimental::Mesh const> getYLineMesh();
        std::shared_ptr<experimental::Mesh const> getTexturedQuadMesh();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
