#pragma once

#include <oscar/Graphics/Mesh.h>

#include <functional>
#include <memory>
#include <string>

namespace osc { class BVH; }

namespace osc
{
    class SceneCache final {
    public:
        SceneCache();
        SceneCache(SceneCache const&) = delete;
        SceneCache(SceneCache&&) noexcept;
        SceneCache& operator=(SceneCache const&) = delete;
        SceneCache& operator=(SceneCache&&) noexcept;
        ~SceneCache() noexcept;

        // clear all cached meshes (can be slow: forces a full reload)
        void clear();

        // always returns (it will use a dummy cube and print a log error if something fails)
        Mesh get(std::string const& key, std::function<Mesh()> const& getter);

        Mesh getSphereMesh();
        Mesh getCircleMesh();
        Mesh getCylinderMesh();
        Mesh getUncappedCylinderMesh();
        Mesh getBrickMesh();
        Mesh getConeMesh();
        Mesh getFloorMesh();
        Mesh get100x100GridMesh();
        Mesh getCubeWireMesh();
        Mesh getYLineMesh();
        Mesh getTexturedQuadMesh();
        Mesh getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius);

        BVH const& getBVH(Mesh const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
