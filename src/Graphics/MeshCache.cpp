#include "MeshCache.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class osc::MeshCache::Impl final {
public:
    std::shared_ptr<Mesh const> sphere = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<Mesh const> cylinder = std::make_shared<Mesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<Mesh const> cube = std::make_shared<Mesh>(GenCube());
    std::shared_ptr<Mesh const> cone = std::make_shared<Mesh>(GenUntexturedSimbodyCone(12));
    std::shared_ptr<Mesh const> floor = std::make_shared<Mesh>(GenTexturedQuad());
    std::shared_ptr<Mesh const> grid100x100 = std::make_shared<Mesh>(GenNbyNGrid(1000));
    std::shared_ptr<Mesh const> cubeWire = std::make_shared<Mesh>(GenCubeLines());
    std::shared_ptr<Mesh const> yLine = std::make_shared<Mesh>(GenYLine());
    std::shared_ptr<Mesh const> texturedQuad = std::make_shared<Mesh>(GenTexturedQuad());

    SynchronizedValue<std::unordered_map<std::string, std::shared_ptr<Mesh const>>> fileCache;
};

osc::MeshCache::MeshCache() :
    m_Impl{new Impl{}}
{
}

osc::MeshCache::MeshCache(MeshCache&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshCache& osc::MeshCache::operator=(MeshCache&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MeshCache::~MeshCache() noexcept
{
    delete m_Impl;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::get(std::string const& key, std::function<std::shared_ptr<osc::Mesh const>()> const& getter)
{
    auto guard = m_Impl->fileCache.lock();

    auto [it, inserted] = guard->try_emplace(key, nullptr);

    if (inserted)
    {
        try
        {
            it->second = getter();
            if (!it->second)
            {
                log::error("%s: a mesh getter returned null: it will be replaced with a dummy cube", key.c_str());
                it->second = m_Impl->cube;  // dummy entry
            }
        }
        catch (std::exception const& ex)
        {
            log::error("%s: error getting a mesh via a getter: it will be replaced with a dummy cube: %s", key.c_str(), ex.what());
            it->second = m_Impl->cube;  // dummy entry
        }
    }

    return it->second;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getConeMesh()
{
    return m_Impl->cone;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

std::shared_ptr<osc::Mesh const> osc::MeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}
