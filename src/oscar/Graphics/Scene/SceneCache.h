#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Platform/ResourcePath.h>

#include <functional>
#include <memory>
#include <string>

namespace osc { class BVH; }
namespace osc { class MeshBasicMaterial; }
namespace osc { class ResourceLoader; }
namespace osc { class Shader; }

namespace osc
{
    class SceneCache final {
    public:
        SceneCache();
        explicit SceneCache(const ResourceLoader&);
        SceneCache(const SceneCache&) = delete;
        SceneCache(SceneCache&&) noexcept;
        SceneCache& operator=(const SceneCache&) = delete;
        SceneCache& operator=(SceneCache&&) noexcept;
        ~SceneCache() noexcept;

        // clear all cached meshes (can be slow: forces a full reload)
        void clearMeshes();

        // always returns (it will use a dummy cube and print a log error if something fails)
        Mesh get(const std::string& key, const std::function<Mesh()>& getter);

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

        const BVH& getBVH(const Mesh&);

        const Shader& getShaderResource(
            const ResourcePath& vertexShader,
            const ResourcePath& fragmentShader
        );

        const Shader& getShaderResource(
            const ResourcePath& vertexShader,
            const ResourcePath& geometryShader,
            const ResourcePath& fragmentShader
        );

        const MeshBasicMaterial& basicMaterial();
        const MeshBasicMaterial& wireframeMaterial();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
