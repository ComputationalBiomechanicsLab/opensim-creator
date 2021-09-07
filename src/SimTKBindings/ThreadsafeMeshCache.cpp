#include "ThreadsafeMeshCache.hpp"

#include "src/SimTKBindings/SimTKLoadMesh.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace osc;

struct osc::ThreadsafeMeshCache::Impl final {
    std::shared_ptr<SceneMesh> sphere = std::make_shared<SceneMesh>(GenUntexturedUVSphere(12, 12));
    std::shared_ptr<SceneMesh> cylinder = std::make_shared<SceneMesh>(GenUntexturedSimbodyCylinder(16));
    std::shared_ptr<SceneMesh> cube = std::make_shared<SceneMesh>(GenCube());
    std::shared_ptr<SceneMesh> cone = std::make_shared<SceneMesh>(GenUntexturedSimbodyCone(12));
    std::mutex fileCacheMutex;
    std::unordered_map<std::string, std::shared_ptr<SceneMesh>> fileCache;
};

osc::ThreadsafeMeshCache::ThreadsafeMeshCache() :
    m_Impl{new Impl{}} {
}

osc::ThreadsafeMeshCache::~ThreadsafeMeshCache() noexcept = default;

std::shared_ptr<SceneMesh> osc::ThreadsafeMeshCache::getMeshFile(std::string const& p) {
    auto guard = std::lock_guard{m_Impl->fileCacheMutex};

    auto [it, inserted] = m_Impl->fileCache.try_emplace(p, nullptr);

    if (inserted) {
        try {
            it->second = std::make_shared<SceneMesh>(SimTKLoadMesh(p));
        } catch (...) {
            m_Impl->fileCache.erase(it);
        }
    }

    return it->second;
}

std::shared_ptr<SceneMesh> osc::ThreadsafeMeshCache::getSphereMesh() {
    return m_Impl->sphere;
}

std::shared_ptr<SceneMesh> osc::ThreadsafeMeshCache::getCylinderMesh() {
    return m_Impl->cylinder;
}

std::shared_ptr<osc::SceneMesh> osc::ThreadsafeMeshCache::getBrickMesh() {
    return m_Impl->cube;
}

std::shared_ptr<SceneMesh> osc::ThreadsafeMeshCache::getConeMesh() {
    return m_Impl->cone;
}
