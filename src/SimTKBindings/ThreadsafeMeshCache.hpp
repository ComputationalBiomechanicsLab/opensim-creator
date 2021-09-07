#pragma once

#include "src/3D/ImmutableSceneMesh.hpp"
#include "src/Assertions.hpp"

#include <memory>
#include <string>

namespace osc {
    class ThreadsafeMeshCache final {

    public:
        [[nodiscard]] static std::shared_ptr<ThreadsafeMeshCache> getGlobal();

        ThreadsafeMeshCache();
        ~ThreadsafeMeshCache() noexcept;

        std::shared_ptr<ImmutableSceneMesh> getMeshFile(std::string const&);  // returns `nullptr` if file could not be loaded
        std::shared_ptr<ImmutableSceneMesh> getSphereMesh();
        std::shared_ptr<ImmutableSceneMesh> getCylinderMesh();
        std::shared_ptr<ImmutableSceneMesh> getBrickMesh();
        std::shared_ptr<ImmutableSceneMesh> getConeMesh();

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
