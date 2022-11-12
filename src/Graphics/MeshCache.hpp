#pragma once

#include <functional>
#include <memory>
#include <string>

namespace osc { class Mesh; }

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

        // always returns non-nullptr (it will use a dummy cube and print a log error if something fails)
        std::shared_ptr<Mesh const> get(std::string const& key, std::function<std::shared_ptr<Mesh const>()> const& getter);

        std::shared_ptr<Mesh const> getSphereMesh();
        std::shared_ptr<Mesh const> getCylinderMesh();
        std::shared_ptr<Mesh const> getBrickMesh();
        std::shared_ptr<Mesh const> getConeMesh();
        std::shared_ptr<Mesh const> getFloorMesh();
        std::shared_ptr<Mesh const> get100x100GridMesh();
        std::shared_ptr<Mesh const> getCubeWireMesh();
        std::shared_ptr<Mesh const> getYLineMesh();
        std::shared_ptr<Mesh const> getTexturedQuadMesh();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
