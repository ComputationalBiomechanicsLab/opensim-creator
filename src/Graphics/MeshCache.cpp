#include "MeshCache.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

class osc::MeshCache::Impl final {
public:
    std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(GenCube());
    std::shared_ptr<Mesh> cone = std::make_shared<Mesh>(GenUntexturedSimbodyCone(12));
    std::shared_ptr<Mesh> floor = []()
    {
        std::shared_ptr<Mesh> rv = std::make_shared<Mesh>(GenTexturedQuad());
        rv->scaleTexCoords(200.0f);
        return rv;
    }();
    std::shared_ptr<Mesh> grid100x100 = std::make_shared<Mesh>(GenNbyNGrid(1000));
    std::shared_ptr<Mesh> cubeWire = std::make_shared<Mesh>(GenCubeLines());
    std::shared_ptr<Mesh> yLine = std::make_shared<Mesh>(GenYLine());
    std::shared_ptr<Mesh> texturedQuad = std::make_shared<Mesh>(GenTexturedQuad());

    SynchronizedValue<std::unordered_map<std::string, std::shared_ptr<Mesh>>> fileCache;

    Impl()
    {
        sphere->setName("Sphere");
        cylinder->setName("Cylinder");
        cube->setName("Cube");
        cone->setName("Cone");
        floor->setName("Floor");
        grid100x100->setName("Grid");
        cubeWire->setName("CubeWireframe");
        yLine->setName("YLine");
        texturedQuad->setName("TexturedQuad");
    }
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

std::shared_ptr<osc::Mesh> osc::MeshCache::getMeshFile(std::string const& p)
{
    auto guard = m_Impl->fileCache.lock();

    auto [it, inserted] = guard->try_emplace(p, nullptr);

    if (inserted)
    {
        try
        {
            auto mesh = std::make_shared<Mesh>(osc::LoadMeshViaSimTK(p));
            mesh->setName(std::filesystem::path{p}.filename().string());
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

std::shared_ptr<osc::Mesh> osc::MeshCache::getSphereMesh()
{
    return m_Impl->sphere;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getCylinderMesh()
{
    return m_Impl->cylinder;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getBrickMesh()
{
    return m_Impl->cube;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getConeMesh()
{
    return m_Impl->cone;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getFloorMesh()
{
    return m_Impl->floor;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::get100x100GridMesh()
{
    return m_Impl->grid100x100;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getCubeWireMesh()
{
    return m_Impl->cubeWire;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getYLineMesh()
{
    return m_Impl->yLine;
}

std::shared_ptr<osc::Mesh> osc::MeshCache::getTexturedQuadMesh()
{
    return m_Impl->texturedQuad;
}
