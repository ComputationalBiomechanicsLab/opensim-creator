#pragma once

#include "src/3D/Mesh.hpp"

#include <memory>
#include <string>

namespace osc {
    class ThreadsafeMeshCache final {

    public:
        [[nodiscard]] static std::shared_ptr<ThreadsafeMeshCache> getGlobalMeshCache();

        ThreadsafeMeshCache();
        ~ThreadsafeMeshCache() noexcept;

        std::shared_ptr<Mesh> getMeshFile(std::string const&);  // returns `nullptr` if file could not be loaded
        std::shared_ptr<Mesh> getSphereMesh();
        std::shared_ptr<Mesh> getCylinderMesh();
        std::shared_ptr<Mesh> getBrickMesh();
        std::shared_ptr<Mesh> getConeMesh();

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
