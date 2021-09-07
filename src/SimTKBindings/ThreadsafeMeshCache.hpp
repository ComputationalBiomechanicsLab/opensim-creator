#pragma once

#include "src/3D/SceneMesh.hpp"
#include "src/Assertions.hpp"

#include <memory>
#include <string>

namespace osc {
    class ThreadsafeMeshCache final {

    public:
        [[nodiscard]] static std::shared_ptr<ThreadsafeMeshCache> getGlobal();

        ThreadsafeMeshCache();
        ~ThreadsafeMeshCache() noexcept;

        std::shared_ptr<SceneMesh> getMeshFile(std::string const&);  // returns `nullptr` if file could not be loaded
        std::shared_ptr<SceneMesh> getSphereMesh();
        std::shared_ptr<SceneMesh> getCylinderMesh();
        std::shared_ptr<SceneMesh> getBrickMesh();
        std::shared_ptr<SceneMesh> getConeMesh();

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
