#pragma once

#include "src/3D/Mesh.hpp"
#include "src/MeshCache.hpp"

#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>
#include <SimTKcommon/internal/DecorativeGeometry.h>

#include <memory>
#include <string>

namespace SimTK {
    class SimbodyMatterSubsystem;
    class State;
}

namespace osc {
    struct SceneElement {
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 color;
        AABB worldspaceAABB;
    };

    class SceneGeneratorNew : public SimTK::DecorativeGeometryImplementation {
        MeshCache& m_MeshCache;
        SimTK::SimbodyMatterSubsystem const& m_Matter;
        SimTK::State const& m_St;
        float m_FixupScaleFactor;

    public:
        SceneGeneratorNew(MeshCache&,
                          SimTK::SimbodyMatterSubsystem const&,
                          SimTK::State const&,
                          float fixupScaleFactor);

    private:
        virtual void onSceneElementEmission(SceneElement const&) = 0;

        void implementPointGeometry(SimTK::DecorativePoint const&) override final;
        void implementLineGeometry(SimTK::DecorativeLine const&) override final;
        void implementBrickGeometry(SimTK::DecorativeBrick const&) override final;
        void implementCylinderGeometry(SimTK::DecorativeCylinder const&) override final;
        void implementCircleGeometry(SimTK::DecorativeCircle const&) override final;
        void implementSphereGeometry(SimTK::DecorativeSphere const&) override final;
        void implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const&) override final;
        void implementFrameGeometry(SimTK::DecorativeFrame const&) override final;
        void implementTextGeometry(SimTK::DecorativeText const&) override final;
        void implementMeshGeometry(SimTK::DecorativeMesh const&) override final;
        void implementMeshFileGeometry(SimTK::DecorativeMeshFile const&) override final;
        void implementArrowGeometry(SimTK::DecorativeArrow const&) override final;
        void implementTorusGeometry(SimTK::DecorativeTorus const&) override final;
        void implementConeGeometry(SimTK::DecorativeCone const&) override final;
    };

    template<typename Callback>
    class SceneGeneratorLambda final : public SceneGeneratorNew {
        Callback m_Callback;
    public:
        SceneGeneratorLambda(MeshCache& meshCache,
                             SimTK::SimbodyMatterSubsystem const& matter,
                             SimTK::State const& st,
                             float fixupScaleFactor,
                             Callback callback) :
            SceneGeneratorNew{meshCache, matter, st, fixupScaleFactor},
            m_Callback{std::move(callback)} {
        }

        void onSceneElementEmission(SceneElement const& se) override {
            m_Callback(se);
        }
    };
}
