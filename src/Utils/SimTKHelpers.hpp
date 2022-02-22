#pragma once

#include "src/3D/Model.hpp"
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

    class DecorativeGeometryHandler final {
    public:
        DecorativeGeometryHandler(MeshCache& meshCache,
                                  SimTK::SimbodyMatterSubsystem const& matter,
                                  SimTK::State const& state,
                                  float fixupScaleFactor,
                                  std::function<void(SystemDecoration const&)>& callback) :
            m_MeshCache{meshCache},
            m_Matter{matter},
            m_State{state},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Callback{callback}
        {
        }

        void operator()(SimTK::DecorativeGeometry const&);

    private:
        MeshCache& m_MeshCache;
        SimTK::SimbodyMatterSubsystem const& m_Matter;
        SimTK::State const& m_State;
        float m_FixupScaleFactor;
        std::function<void(SystemDecoration const&)>& m_Callback;
    };
}
