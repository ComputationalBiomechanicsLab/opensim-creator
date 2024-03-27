#include "SceneCache.h"

#include <oscar/Graphics/Geometries.h>
#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Platform/FilesystemResourceLoader.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    struct TorusParameters final {

        TorusParameters(
            float torusCenterToTubeCenterRadius_,
            float tubeRadius_) :

            torusCenterToTubeCenterRadius{torusCenterToTubeCenterRadius_},
            tubeRadius{tubeRadius_}
        {}

        friend bool operator==(const TorusParameters&, const TorusParameters&) = default;

        float torusCenterToTubeCenterRadius;
        float tubeRadius;
    };

    // parameters for a shader, to be used as a key into the shader cache
    struct ShaderInputs final {

        ShaderInputs(
            ResourcePath vertexShaderPath_,
            ResourcePath fragmentShaderPath_) :

            vertexShaderPath{std::move(vertexShaderPath_)},
            fragmentShaderPath{std::move(fragmentShaderPath_)}
        {}

        ShaderInputs(
            ResourcePath vertexShaderPath_,
            ResourcePath geometryShaderPath_,
            ResourcePath fragmentShaderPath_) :

            vertexShaderPath{std::move(vertexShaderPath_)},
            geometryShaderPath{std::move(geometryShaderPath_)},
            fragmentShaderPath{std::move(fragmentShaderPath_)}
        {}

        friend bool operator==(const ShaderInputs&, const ShaderInputs&) = default;

        ResourcePath vertexShaderPath;
        ResourcePath geometryShaderPath;
        ResourcePath fragmentShaderPath;

        size_t hash = HashOf(vertexShaderPath, geometryShaderPath, fragmentShaderPath);
    };

    Mesh generateYToYLineMesh()
    {
        Mesh rv;
        rv.setTopology(MeshTopology::Lines);
        rv.setVerts({{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}});
        rv.setNormals({{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}});  // just give them *something* in-case they are rendered through a shader that requires normals
        rv.setIndices({0, 1});
        return rv;
    }
}

template<>
struct std::hash<ShaderInputs> final {

    size_t operator()(const ShaderInputs& inputs) const
    {
        return inputs.hash;
    }
};

template<>
struct std::hash<TorusParameters> final {
    size_t operator()(const TorusParameters& p) const
    {
        return HashOf(p.torusCenterToTubeCenterRadius, p.tubeRadius);
    }
};

class osc::SceneCache::Impl final {
public:
    Impl() :
        Impl{make_resource_loader<FilesystemResourceLoader>(".")}
    {}

    explicit Impl(ResourceLoader resourceLoader_) :
        m_Loader{std::move(resourceLoader_)}
    {}

    const Shader& load(
        const ResourcePath& vertexShader,
        const ResourcePath& fragmentShader)
    {
        const ShaderInputs key{vertexShader, fragmentShader};

        auto guard = m_Cache.lock();
        const auto it = guard->find(key);
        if (it != guard->end()) {
            return it->second;
        }
        else {
            const std::string vertexShaderSrc = m_Loader.slurp(key.vertexShaderPath);
            const std::string fragmentShaderSrc = m_Loader.slurp(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, fragmentShaderSrc})->second;
        }
    }
    const Shader& load(
        const ResourcePath& vertexShader,
        const ResourcePath& geometryShader,
        const ResourcePath& fragmentShader)
    {
        const ShaderInputs key{vertexShader, geometryShader, fragmentShader};

        auto guard = m_Cache.lock();
        const auto it = guard->find(key);
        if (it != guard->end()) {
            return it->second;
        }
        else {
            const std::string vertexShaderSrc = m_Loader.slurp(key.vertexShaderPath);
            const std::string geometryShaderSrc = m_Loader.slurp(key.geometryShaderPath);
            const std::string fragmentShaderSrc = m_Loader.slurp(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, geometryShaderSrc, fragmentShaderSrc})->second;
        }
    }

    const MeshBasicMaterial& basicMaterial()
    {
        return m_BasicMaterial.emplace();
    }

    const MeshBasicMaterial& wireframeMaterial()
    {
        if (not m_WireframeMaterial) {
            m_WireframeMaterial.emplace();
            m_WireframeMaterial->setColor({0.0f, 0.6f});
            m_WireframeMaterial->setWireframeMode(true);
            m_WireframeMaterial->setTransparent(true);
        }
        return *m_WireframeMaterial;
    }

    Mesh sphere = SphereGeometry{1.0f, 16, 16};
    Mesh circle = CircleGeometry{1.0f, 16};
    Mesh cylinder = CylinderGeometry{1.0f, 1.0f, 2.0f, 16};
    Mesh uncappedCylinder = CylinderGeometry{1.0f, 1.0f, 2.0f, 16, 1, true};
    Mesh cube = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh cone = ConeGeometry{1.0f, 2.0f, 16};
    Mesh floor = PlaneGeometry{2.0f, 2.0f, 1, 1};
    Mesh grid100x100 = GridGeometry{2.0f, 1000};
    Mesh cubeWire = AABBGeometry{};
    Mesh yLine = generateYToYLineMesh();
    Mesh texturedQuad = floor;

    SynchronizedValue<std::unordered_map<TorusParameters, Mesh>> torusCache;
    SynchronizedValue<std::unordered_map<std::string, Mesh>> fileCache;
    SynchronizedValue<std::unordered_map<Mesh, std::unique_ptr<BVH>>> bvhCache;

    // shader stuff
    ResourceLoader m_Loader;
    SynchronizedValue<std::unordered_map<ShaderInputs, Shader>> m_Cache;
    std::optional<MeshBasicMaterial> m_BasicMaterial;
    std::optional<MeshBasicMaterial> m_WireframeMaterial;
};

osc::SceneCache::SceneCache() :
    m_Impl{std::make_unique<Impl>()}
{}

osc::SceneCache::SceneCache(const ResourceLoader& loader_) :
    m_Impl{std::make_unique<Impl>(loader_)}
{}

osc::SceneCache::SceneCache(SceneCache&&) noexcept = default;
osc::SceneCache& osc::SceneCache::operator=(SceneCache&&) noexcept = default;
osc::SceneCache::~SceneCache() noexcept = default;

void osc::SceneCache::clearMeshes()
{
    m_Impl->fileCache.lock()->clear();
    m_Impl->bvhCache.lock()->clear();
    m_Impl->torusCache.lock()->clear();
}

Mesh osc::SceneCache::get(
    const std::string& key,
    const std::function<Mesh()>& getter)
{
    auto guard = m_Impl->fileCache.lock();

    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);
    if (inserted) {
        try {
            it->second = getter();
        }
        catch (const std::exception& ex) {
            log_error("%s: error getting a mesh via a getter: it will be replaced with a dummy cube: %s", key.c_str(), ex.what());
        }
    }

    return it->second;
}

Mesh osc::SceneCache::getSphereMesh()
{
    return m_Impl->sphere;
}

Mesh osc::SceneCache::getCircleMesh()
{
    return m_Impl->circle;
}

Mesh osc::SceneCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

Mesh osc::SceneCache::getUncappedCylinderMesh()
{
    return m_Impl->uncappedCylinder;
}

Mesh osc::SceneCache::getBrickMesh()
{
    return m_Impl->cube;
}

Mesh osc::SceneCache::getConeMesh()
{
    return m_Impl->cone;
}

Mesh osc::SceneCache::getFloorMesh()
{
    return m_Impl->floor;
}

Mesh osc::SceneCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

Mesh osc::SceneCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

Mesh osc::SceneCache::getYLineMesh()
{
    return m_Impl->yLine;
}

Mesh osc::SceneCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}

Mesh osc::SceneCache::getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius)
{
    const TorusParameters key{torusCenterToTubeCenterRadius, tubeRadius};

    auto guard = m_Impl->torusCache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);
    if (inserted) {
        it->second = TorusGeometry{key.torusCenterToTubeCenterRadius, key.tubeRadius, 12, 12, Degrees{360}};
    }

    return it->second;
}

const BVH& osc::SceneCache::getBVH(const Mesh& mesh)
{
    auto guard = m_Impl->bvhCache.lock();
    auto [it, inserted] = guard->try_emplace(mesh, nullptr);
    if (inserted) {
        it->second = std::make_unique<BVH>(createTriangleBVHFromMesh(mesh));
    }
    return *it->second;
}

const Shader& osc::SceneCache::getShaderResource(
    const ResourcePath& vertexShader,
    const ResourcePath& fragmentShader)
{
    return m_Impl->load(vertexShader, fragmentShader);
}

const Shader& osc::SceneCache::getShaderResource(
    const ResourcePath& vertexShader,
    const ResourcePath& geometryShader,
    const ResourcePath& fragmentShader)
{
    return m_Impl->load(vertexShader, geometryShader, fragmentShader);
}

const MeshBasicMaterial& osc::SceneCache::basicMaterial()
{
    return m_Impl->basicMaterial();
}

const MeshBasicMaterial& osc::SceneCache::wireframeMaterial()
{
    return m_Impl->wireframeMaterial();
}
