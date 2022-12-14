#pragma once

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/Rotation.h>

#include <filesystem>
#include <memory>

namespace osc { class MeshCache; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    // converters: from osc types to SimTK
    SimTK::Vec3 ToSimTKVec3(float v[3]);
    SimTK::Vec3 ToSimTKVec3(glm::vec3 const&);
    SimTK::Mat33 ToSimTKMat3(glm::mat3 const&);
    SimTK::Inertia ToSimTKInertia(float v[3]);
    SimTK::Inertia ToSimTKInertia(glm::vec3 const&);
    SimTK::Transform ToSimTKTransform(glm::mat4x3 const&);
    SimTK::Transform ToSimTKTransform(Transform const&);
    SimTK::Rotation ToSimTKRotation(glm::quat const&);

    // converters: from SimTK types to osc
    glm::vec3 ToVec3(SimTK::Vec3 const&);
    glm::vec4 ToVec4(SimTK::Vec3 const&, float w = 1.0f);
    glm::mat4x3 ToMat4x3(SimTK::Transform const&);
    glm::mat4x4 ToMat4x4(SimTK::Transform const&);
    glm::quat ToQuat(SimTK::Rotation const&);
    Transform ToTransform(SimTK::Transform const&);

    // mesh loading
    Mesh LoadMeshViaSimTK(std::filesystem::path const&);

    // rendering

    // called with an appropriate (output) decoration whenever the
    // DecorativeGeometryHandler wants to emit geometry
    class DecorationConsumer {
    public:
        DecorationConsumer() = default;
        DecorationConsumer(DecorationConsumer const&) = delete;
        DecorationConsumer(DecorationConsumer&&) noexcept = delete;
        DecorationConsumer& operator=(DecorationConsumer const&) = delete;
        DecorationConsumer& operator=(DecorationConsumer&&) noexcept = delete;
        virtual ~DecorationConsumer() noexcept = default;

        virtual void operator()(Mesh const&, Transform const&, glm::vec4 const& color) = 0;
    };

    // consumes SimTK::DecorativeGeometry and emits appropriate decorations back to
    // the `DecorationConsumer`
    class DecorativeGeometryHandler final {
    public:
        DecorativeGeometryHandler(MeshCache& meshCache,
                                  SimTK::SimbodyMatterSubsystem const& matter,
                                  SimTK::State const& state,
                                  float fixupScaleFactor,
                                  DecorationConsumer&);
        DecorativeGeometryHandler(DecorativeGeometryHandler const&) = delete;
        DecorativeGeometryHandler(DecorativeGeometryHandler&&) noexcept;
        DecorativeGeometryHandler& operator=(DecorativeGeometryHandler const&) = delete;
        DecorativeGeometryHandler& operator=(DecorativeGeometryHandler&&) noexcept;
        ~DecorativeGeometryHandler() noexcept;

        void operator()(SimTK::DecorativeGeometry const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
