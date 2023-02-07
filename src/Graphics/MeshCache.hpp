#pragma once

#include "src/Graphics/Mesh.hpp"

#include <functional>
#include <memory>
#include <string>

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

        // clear all cached meshes (can be slow: forces a full reload)
        void clear();

        // always returns (it will use a dummy cube and print a log error if something fails)
        Mesh get(std::string const& key, std::function<Mesh()> const& getter);

        Mesh getSphereMesh();
        Mesh getCircleMesh();
        Mesh getCylinderMesh();
        Mesh getBrickMesh();
        Mesh getConeMesh();
        Mesh getFloorMesh();
        Mesh get100x100GridMesh();
        Mesh getCubeWireMesh();
        Mesh getYLineMesh();
        Mesh getTexturedQuadMesh();
        Mesh getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
