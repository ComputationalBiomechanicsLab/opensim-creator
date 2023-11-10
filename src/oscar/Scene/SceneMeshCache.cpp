#include "SceneMeshCache.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

        friend bool operator==(TorusParameters const&, TorusParameters const&) noexcept = default;

        float torusCenterToTubeCenterRadius;
        float tubeRadius;
    };
}

template<>
struct std::hash<TorusParameters> final {
    size_t operator()(TorusParameters const& p) const noexcept
    {
        return osc::HashOf(p.torusCenterToTubeCenterRadius, p.tubeRadius);
    }
};

class osc::SceneMeshCache::Impl final {
public:
    SceneMesh sphere{GenSphere(16, 16)};
    SceneMesh circle{GenCircle(16)};
    SceneMesh cylinder{GenUntexturedYToYCylinder(16)};
    SceneMesh cube{GenCube()};
    SceneMesh cone{GenUntexturedYToYCone(16)};
    SceneMesh floor{GenTexturedQuad()};
    SceneMesh grid100x100{GenNbyNGrid(1000)};
    SceneMesh cubeWire{GenCubeLines()};
    SceneMesh yLine{GenYLine()};
    SceneMesh texturedQuad{GenTexturedQuad()};

    SynchronizedValue<std::unordered_map<TorusParameters, SceneMesh>> torusCache;
    SynchronizedValue<std::unordered_map<std::string, SceneMesh>> fileCache;
};

osc::SceneMeshCache::SceneMeshCache() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::SceneMeshCache::SceneMeshCache(SceneMeshCache&&) noexcept = default;
osc::SceneMeshCache& osc::SceneMeshCache::operator=(SceneMeshCache&&) noexcept = default;
osc::SceneMeshCache::~SceneMeshCache() noexcept = default;

void osc::SceneMeshCache::clear()
{
    m_Impl->fileCache.lock()->clear();
}

osc::SceneMesh osc::SceneMeshCache::get(
    std::string const& key,
    std::function<SceneMesh()> const& getter)
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

osc::SceneMesh osc::SceneMeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

osc::SceneMesh osc::SceneMeshCache::getCircleMesh()
{
    return m_Impl->circle;
}

osc::SceneMesh osc::SceneMeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

osc::SceneMesh osc::SceneMeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

osc::SceneMesh osc::SceneMeshCache::getConeMesh()
{
    return m_Impl->cone;
}

osc::SceneMesh osc::SceneMeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

osc::SceneMesh osc::SceneMeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

osc::SceneMesh osc::SceneMeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

osc::SceneMesh osc::SceneMeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

osc::SceneMesh osc::SceneMeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}

osc::SceneMesh osc::SceneMeshCache::getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius)
{
    TorusParameters const key{torusCenterToTubeCenterRadius, tubeRadius};

    auto guard = m_Impl->torusCache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);

    if (inserted)
    {
        it->second = SceneMesh{GenTorus(12, 12, key.torusCenterToTubeCenterRadius, key.tubeRadius)};
    }

    return it->second;
}
