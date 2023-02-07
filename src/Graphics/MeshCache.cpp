#include "MeshCache.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/SynchronizedValue.hpp"

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

        float torusCenterToTubeCenterRadius;
        float tubeRadius;
    };

    bool operator==(TorusParameters const& a, TorusParameters const& b) noexcept
    {
        return a.torusCenterToTubeCenterRadius == b.torusCenterToTubeCenterRadius && a.tubeRadius == b.tubeRadius;
    }
}

namespace std
{
    template<>
    struct hash<TorusParameters> final {
        size_t operator()(TorusParameters const& p) const noexcept
        {
            return osc::HashOf(p.torusCenterToTubeCenterRadius, p.tubeRadius);
        }
    };
}

class osc::MeshCache::Impl final {
public:
    Mesh sphere = GenUntexturedUVSphere(12, 12);
    Mesh circle = GenCircle(16);
    Mesh cylinder = GenUntexturedSimbodyCylinder(16);
    Mesh cube = GenCube();
    Mesh cone = GenUntexturedSimbodyCone(12);
    Mesh floor = GenTexturedQuad();
    Mesh grid100x100 = GenNbyNGrid(1000);
    Mesh cubeWire = GenCubeLines();
    Mesh yLine = GenYLine();
    Mesh texturedQuad = GenTexturedQuad();

    SynchronizedValue<std::unordered_map<TorusParameters, Mesh>> torusCache;
    SynchronizedValue<std::unordered_map<std::string, Mesh>> fileCache;
};

osc::MeshCache::MeshCache() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MeshCache::MeshCache(MeshCache&&) noexcept = default;
osc::MeshCache& osc::MeshCache::operator=(MeshCache&&) noexcept = default;
osc::MeshCache::~MeshCache() noexcept = default;

void osc::MeshCache::clear()
{
    m_Impl->fileCache.lock()->clear();
}

osc::Mesh osc::MeshCache::get(std::string const& key, std::function<Mesh()> const& getter)
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

osc::Mesh osc::MeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

osc::Mesh osc::MeshCache::getCircleMesh()
{
    return m_Impl->circle;
}

osc::Mesh osc::MeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

osc::Mesh osc::MeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

osc::Mesh osc::MeshCache::getConeMesh()
{
    return m_Impl->cone;
}

osc::Mesh osc::MeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

osc::Mesh osc::MeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

osc::Mesh osc::MeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

osc::Mesh osc::MeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

osc::Mesh osc::MeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}

osc::Mesh osc::MeshCache::getTorusMesh(float torusCenterToTubeCenterRadius, float tubeRadius)
{
    TorusParameters const key{torusCenterToTubeCenterRadius, tubeRadius};

    auto guard = m_Impl->torusCache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);

    if (inserted)
    {
        it->second = GenTorus(12, 12, key.torusCenterToTubeCenterRadius, key.tubeRadius);
    }

    return it->second;
}
