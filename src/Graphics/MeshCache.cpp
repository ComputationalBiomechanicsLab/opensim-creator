#include "MeshCache.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class osc::MeshCache::Impl final {
public:
    std::shared_ptr<experimental::Mesh const> sphere = std::make_shared<experimental::Mesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<experimental::Mesh const> cylinder = std::make_shared<experimental::Mesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<experimental::Mesh const> cube = std::make_shared<experimental::Mesh>(GenCube());
    std::shared_ptr<experimental::Mesh const> cone = std::make_shared<experimental::Mesh>(GenUntexturedSimbodyCone(12));
    std::shared_ptr<experimental::Mesh const> floor = []()
    {
        std::shared_ptr<experimental::Mesh> rv = std::make_shared<experimental::Mesh>(GenTexturedQuad());
        nonstd::span<glm::vec2 const> oldCoords = rv->getTexCoords();
        std::vector<glm::vec2> newCoords;
        newCoords.reserve(oldCoords.size());
        for (glm::vec2 const& oldCoord : oldCoords)
        {
            newCoords.push_back(200.0f * oldCoord);
        }
        return rv;
    }();
    std::shared_ptr<experimental::Mesh const> grid100x100 = std::make_shared<experimental::Mesh>(GenNbyNGrid(1000));
    std::shared_ptr<experimental::Mesh const> cubeWire = std::make_shared<experimental::Mesh>(GenCubeLines());
    std::shared_ptr<experimental::Mesh const> yLine = std::make_shared<experimental::Mesh>(GenYLine());
    std::shared_ptr<experimental::Mesh const> texturedQuad = std::make_shared<experimental::Mesh>(GenTexturedQuad());

    SynchronizedValue<std::unordered_map<std::string, std::shared_ptr<experimental::Mesh const>>> fileCache;
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

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getMeshFile(std::string const& p)
{
    auto guard = m_Impl->fileCache.lock();

    auto [it, inserted] = guard->try_emplace(p, nullptr);

    if (inserted)
    {
        try
        {
            auto mesh = std::make_shared<experimental::Mesh>(osc::LoadMeshViaSimTK(p));
            it->second = std::move(mesh);
        }
        catch (...)
        {
            log::error("error loading mesh file %s: it will be replaced with a cube", p.c_str());
            it->second = m_Impl->cube;  // dummy entry
        }
    }

    return it->second;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getConeMesh()
{
    return m_Impl->cone;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

std::shared_ptr<osc::experimental::Mesh const> osc::MeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}
