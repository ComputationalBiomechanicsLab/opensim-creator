#include "ThreadsafeMeshCache.hpp"

#include "src/SimTKBindings/SimTKLoadMesh.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace osc;


std::shared_ptr<ThreadsafeMeshCache> osc::ThreadsafeMeshCache::getGlobal() {
    static std::shared_ptr<ThreadsafeMeshCache> g_GlobalMeshCache = std::make_shared<ThreadsafeMeshCache>();
    return g_GlobalMeshCache;
}

struct osc::ThreadsafeMeshCache::Impl final {
    std::shared_ptr<ImmutableSceneMesh> sphere = std::make_shared<ImmutableSceneMesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<ImmutableSceneMesh> cylinder = std::make_shared<ImmutableSceneMesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<ImmutableSceneMesh> cube = std::make_shared<ImmutableSceneMesh>(GenCube());
    std::shared_ptr<ImmutableSceneMesh> cone = std::make_shared<ImmutableSceneMesh>(GenUntexturedSimbodyCone(12));
    std::mutex fileCacheMutex;
    std::unordered_map<std::string, std::shared_ptr<ImmutableSceneMesh>> fileCache;
};

osc::ThreadsafeMeshCache::ThreadsafeMeshCache() :
    m_Impl{new Impl{}} {
}

osc::ThreadsafeMeshCache::~ThreadsafeMeshCache() noexcept = default;

std::shared_ptr<ImmutableSceneMesh> osc::ThreadsafeMeshCache::getMeshFile(std::string const& p) {
    auto guard = std::lock_guard{m_Impl->fileCacheMutex};

    auto [it, inserted] = m_Impl->fileCache.try_emplace(p, nullptr);

    if (inserted) {
        try {
            it->second = std::make_shared<ImmutableSceneMesh>(SimTKLoadMesh(p));
        } catch (...) {
            m_Impl->fileCache.erase(it);
        }
    }

    return it->second;
}

std::shared_ptr<ImmutableSceneMesh> osc::ThreadsafeMeshCache::getSphereMesh() {
    return m_Impl->sphere;
}

std::shared_ptr<ImmutableSceneMesh> osc::ThreadsafeMeshCache::getCylinderMesh() {
    return m_Impl->cylinder;
}

std::shared_ptr<osc::ImmutableSceneMesh> osc::ThreadsafeMeshCache::getBrickMesh() {
    return m_Impl->cube;
}

std::shared_ptr<ImmutableSceneMesh> osc::ThreadsafeMeshCache::getConeMesh() {
    return m_Impl->cone;
}
