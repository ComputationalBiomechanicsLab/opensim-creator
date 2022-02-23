#pragma once

#include "src/3D/Model.hpp"  // Transform, AABB
#include "src/3D/Mesh.hpp"
#include "src/MeshCache.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <SimTKcommon.h>

#include <filesystem>
#include <functional>
#include <memory>

namespace SimTK
{
    class SimbodyMatterSubsystem;
    class State;
}

namespace osc
{
    // converters
    SimTK::Vec3 SimTKVec3FromV3(float v[3]);
    SimTK::Vec3 SimTKVec3FromV3(glm::vec3 const&);
    SimTK::Inertia SimTKInertiaFromV3(float v[3]);
    glm::vec3 SimTKVec3FromVec3(SimTK::Vec3 const&);
    glm::vec4 SimTKVec4FromVec3(SimTK::Vec3 const&, float = 1.0f);
    glm::mat4x3 SimTKMat4x3FromXForm(SimTK::Transform const&);
    glm::mat4x4 SimTKMat4x4FromTransform(SimTK::Transform const&);
    SimTK::Transform SimTKTransformFromMat4x3(glm::mat4x3 const&);


    // mesh loading
    Mesh SimTKLoadMesh(std::filesystem::path const&);


    // rendering

    struct SystemDecoration {
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 color;
        AABB worldspaceAABB;
    };

    // temporary hack while implementing the new handler interface
    struct SystemDecorationNew {
        std::shared_ptr<Mesh> mesh;
        Transform transform;
        glm::vec4 color;

        operator SystemDecoration() const;
    };

    class DecorativeGeometryHandler final {
    public:
        DecorativeGeometryHandler(MeshCache& meshCache,
                                  SimTK::SimbodyMatterSubsystem const& matter,
                                  SimTK::State const& state,
                                  float fixupScaleFactor,
                                  std::function<void(SystemDecorationNew const&)>& callback);
        DecorativeGeometryHandler(DecorativeGeometryHandler const&) = delete;
        DecorativeGeometryHandler(DecorativeGeometryHandler&&) noexcept;
        ~DecorativeGeometryHandler() noexcept;

        DecorativeGeometryHandler& operator=(DecorativeGeometryHandler const&) = delete;
        DecorativeGeometryHandler& operator=(DecorativeGeometryHandler&&) noexcept;

        void operator()(SimTK::DecorativeGeometry const&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
