#pragma once

#include <oscar/Scene/SceneMesh.hpp>

#include <functional>
#include <memory>
#include <string>

namespace osc
{
    class SceneMeshCache final {
    public:
        SceneMeshCache();
        SceneMeshCache(SceneMeshCache const&) = delete;
        SceneMeshCache(SceneMeshCache&&) noexcept;
        SceneMeshCache& operator=(SceneMeshCache const&) = delete;
        SceneMeshCache& operator=(SceneMeshCache&&) noexcept;
        ~SceneMeshCache() noexcept;

        // clear all cached meshes (can be slow: forces a full reload)
        void clear();

        // always returns (it will use a dummy cube and print a log error if something fails)
        SceneMesh get(std::string const& key, std::function<SceneMesh()> const& getter);

        SceneMesh getSphereMesh();
        SceneMesh getCircleMesh();
        SceneMesh getCylinderMesh();
        SceneMesh getBrickMesh();
        SceneMesh getConeMesh();
        SceneMesh getFloorMesh();
        SceneMesh get100x100GridMesh();
        SceneMesh getCubeWireMesh();
        SceneMesh getYLineMesh();
        SceneMesh getTexturedQuadMesh();
        SceneMesh getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}