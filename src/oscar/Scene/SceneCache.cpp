#include "SceneCache.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using osc::BVH;
using osc::Mesh;
using osc::MeshTopology;

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
        return osc::HashOf(p.torusCenterToTubeCenterRadius, p.tubeRadius);
    }
};

class osc::SceneCache::Impl final {
public:
    Impl()
    {
    }

    Mesh sphere{GenSphere(16, 16)};
    Mesh circle{GenCircle(16)};
    Mesh cylinder{GenUntexturedYToYCylinder(16)};
    Mesh cube{GenCube()};
    Mesh cone{GenUntexturedYToYCone(16)};
    Mesh floor{GenTexturedQuad()};
    Mesh grid100x100{GenNbyNGrid(1000)};
    Mesh cubeWire{GenCubeLines()};
    Mesh yLine{GenYLine()};
    Mesh texturedQuad{GenTexturedQuad()};

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

osc::Mesh osc::SceneCache::get(
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
            log::error("%s: error getting a mesh via a getter: it will be replaced with a dummy cube: %s", key.c_str(), ex.what());
        }
    }

    return it->second;
}

osc::Mesh osc::SceneCache::getSphereMesh()
{
    return m_Impl->sphere;
}

osc::Mesh osc::SceneCache::getCircleMesh()
{
    return m_Impl->circle;
}

osc::Mesh osc::SceneCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

osc::Mesh osc::SceneCache::getBrickMesh()
{
    return m_Impl->cube;
}

osc::Mesh osc::SceneCache::getConeMesh()
{
    return m_Impl->cone;
}

osc::Mesh osc::SceneCache::getFloorMesh()
{
    return m_Impl->floor;
}

osc::Mesh osc::SceneCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

osc::Mesh osc::SceneCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

osc::Mesh osc::SceneCache::getYLineMesh()
{
    return m_Impl->yLine;
}

osc::Mesh osc::SceneCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}

osc::Mesh osc::SceneCache::getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius)
{
    TorusParameters const key{torusCenterToTubeCenterRadius, tubeRadius};

    auto guard = m_Impl->torusCache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);

    if (inserted)
    {
        it->second = Mesh{GenTorus(12, 12, key.torusCenterToTubeCenterRadius, key.tubeRadius)};
    }

    return it->second;
}

osc::BVH const& osc::SceneCache::getBVH(Mesh const& mesh)
{
    auto guard = m_Impl->bvhCache.lock();
    auto [it, inserted] = guard->try_emplace(mesh, nullptr);
    if (inserted)
    {
        it->second = std::make_unique<BVH>(CreateTriangleBVHFromMesh(mesh));
    }
    return *it->second;
}
