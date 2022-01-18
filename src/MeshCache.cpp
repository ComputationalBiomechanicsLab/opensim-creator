#include "MeshCache.hpp"

#include "src/3D/Mesh.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/Log.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace osc;

struct osc::MeshCache::Impl final {
    std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(GenCube());
    std::shared_ptr<Mesh> cone = std::make_shared<Mesh>(GenUntexturedSimbodyCone(12));
    std::shared_ptr<Mesh> floor = []() {
        std::shared_ptr<Mesh> rv = std::make_shared<Mesh>(GenTexturedQuad());
        rv->scaleTexCoords(200.0f);
        return rv;
    }();
    std::shared_ptr<Mesh> grid100x100 = std::make_shared<Mesh>(GenNbyNGrid(1000));
    std::shared_ptr<Mesh> cubeWire = std::make_shared<Mesh>(GenCubeLines());
    std::shared_ptr<Mesh> yLine = std::make_shared<Mesh>(GenYLine());
    std::shared_ptr<Mesh> texturedQuad = std::make_shared<Mesh>(GenTexturedQuad());
    std::mutex fileCacheMutex;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> fileCache;
};

osc::MeshCache::MeshCache() :
    m_Impl{new Impl{}}
{
}

osc::MeshCache::~MeshCache() noexcept = default;

std::shared_ptr<Mesh> osc::MeshCache::getMeshFile(std::string const& p)
{
    auto guard = std::lock_guard{m_Impl->fileCacheMutex};

    auto [it, inserted] = m_Impl->fileCache.try_emplace(p, nullptr);

    if (inserted)
    {
        try
        {
            it->second = std::make_shared<Mesh>(SimTKLoadMesh(p));
        }
        catch (...)
        {
            log::error("error loading mesh file %s: it will be replaced with a cube", p.c_str());
            it->second = m_Impl->cube;  // dummy entry
        }
    }

    return it->second;
}

std::shared_ptr<Mesh> osc::MeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

std::shared_ptr<Mesh> osc::MeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

std::shared_ptr<Mesh> osc::MeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

std::shared_ptr<Mesh> osc::MeshCache::getConeMesh()
{
    return m_Impl->cone;
}

std::shared_ptr<Mesh> osc::MeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

std::shared_ptr<Mesh> osc::MeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

std::shared_ptr<Mesh> osc::MeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

std::shared_ptr<Mesh> osc::MeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

std::shared_ptr<Mesh> osc::MeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}
