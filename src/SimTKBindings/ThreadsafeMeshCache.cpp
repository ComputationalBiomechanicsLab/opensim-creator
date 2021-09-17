#include "ThreadsafeMeshCache.hpp"

#include "src/3D/Mesh.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace osc;


std::shared_ptr<ThreadsafeMeshCache> osc::ThreadsafeMeshCache::getGlobalMeshCache() {
    static std::shared_ptr<ThreadsafeMeshCache> g_GlobalMeshCache = std::make_shared<ThreadsafeMeshCache>();
    return g_GlobalMeshCache;
}

struct osc::ThreadsafeMeshCache::Impl final {
    std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(GenCube());
    std::shared_ptr<Mesh> cone = std::make_shared<Mesh>(GenUntexturedSimbodyCone(12));
    std::mutex fileCacheMutex;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> fileCache;
};

osc::ThreadsafeMeshCache::ThreadsafeMeshCache() :
    m_Impl{new Impl{}} {
}

osc::ThreadsafeMeshCache::~ThreadsafeMeshCache() noexcept = default;

std::shared_ptr<Mesh> osc::ThreadsafeMeshCache::getMeshFile(std::string const& p) {
    auto guard = std::lock_guard{m_Impl->fileCacheMutex};

    auto [it, inserted] = m_Impl->fileCache.try_emplace(p, nullptr);

    if (inserted) {
        try {
            it->second = std::make_shared<Mesh>(SimTKLoadMesh(p));
        } catch (...) {
            m_Impl->fileCache.erase(it);
        }
    }

    return it->second;
}

std::shared_ptr<Mesh> osc::ThreadsafeMeshCache::getSphereMesh() {
    return m_Impl->sphere;
}

std::shared_ptr<Mesh> osc::ThreadsafeMeshCache::getCylinderMesh() {
    return m_Impl->cylinder;
}

std::shared_ptr<Mesh> osc::ThreadsafeMeshCache::getBrickMesh() {
    return m_Impl->cube;
}

std::shared_ptr<Mesh> osc::ThreadsafeMeshCache::getConeMesh() {
    return m_Impl->cone;
}
