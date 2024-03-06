#include "SceneCache.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>

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
        {
        }

        friend bool operator==(TorusParameters const&, TorusParameters const&) = default;

        float torusCenterToTubeCenterRadius;
        float tubeRadius;
    };
}

template<>
struct std::hash<TorusParameters> final {
    size_t operator()(TorusParameters const& p) const
    {
        return HashOf(p.torusCenterToTubeCenterRadius, p.tubeRadius);
    }
};

class osc::SceneCache::Impl final {
public:
    Mesh sphere = GenerateSphereMesh2(1.0f, 16, 16);
    Mesh circle = GenerateCircleMesh2(1.0f, 16);
    Mesh cylinder = GenerateCylinderMesh2(1.0f, 1.0f, 2.0f, 16);
    Mesh uncappedCylinder = GenerateCylinderMesh2(1.0f, 1.0f, 2.0f, 16, 1, true);
    Mesh cube = GenerateBoxMesh(2.0f, 2.0f, 2.0f);
    Mesh cone = GenerateConeMesh2(1.0f, 2.0f, 16);
    Mesh floor = GeneratePlaneMesh2(2.0f, 2.0f, 1, 1);
    Mesh grid100x100 = GenerateGridLinesMesh(1000);
    Mesh cubeWire = GenerateCubeLinesMesh();
    Mesh yLine = GenerateYToYLineMesh();
    Mesh texturedQuad = floor;

    SynchronizedValue<std::unordered_map<TorusParameters, Mesh>> torusCache;
    SynchronizedValue<std::unordered_map<std::string, Mesh>> fileCache;
    SynchronizedValue<std::unordered_map<Mesh, std::unique_ptr<BVH>>> bvhCache;
};

osc::SceneCache::SceneCache() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::SceneCache::SceneCache(SceneCache&&) noexcept = default;
osc::SceneCache& osc::SceneCache::operator=(SceneCache&&) noexcept = default;
osc::SceneCache::~SceneCache() noexcept = default;

void osc::SceneCache::clear()
{
    m_Impl->fileCache.lock()->clear();
    m_Impl->bvhCache.lock()->clear();
    m_Impl->torusCache.lock()->clear();
}

Mesh osc::SceneCache::get(
    std::string const& key,
    std::function<Mesh()> const& getter)
{
    auto guard = m_Impl->fileCache.lock();

    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);

    if (inserted)
    {
        try
        {
            it->second = getter();
        }
        catch (std::exception const& ex)
        {
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
    TorusParameters const key{torusCenterToTubeCenterRadius, tubeRadius};

    auto guard = m_Impl->torusCache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);

    if (inserted)
    {
        it->second = Mesh{GenerateTorusMesh2(key.torusCenterToTubeCenterRadius, key.tubeRadius, 12, 12, Degrees{360})};
    }

    return it->second;
}

BVH const& osc::SceneCache::getBVH(Mesh const& mesh)
{
    auto guard = m_Impl->bvhCache.lock();
    auto [it, inserted] = guard->try_emplace(mesh, nullptr);
    if (inserted)
    {
        it->second = std::make_unique<BVH>(CreateTriangleBVHFromMesh(mesh));
    }
    return *it->second;
}
