#pragma once

#include "src/3d/untextured_vert.hpp"

#include "SimTKcommon/internal/DecorativeGeometry.h"

#include <vector>

namespace SimTK {
    class SimbodyMatterSubsystem;
    class State;
}

namespace osmv {
    struct Gpu_cache;
    struct Mesh_instance;
}

namespace osmv {
    class Simbody_geometry_visitor : public SimTK::DecorativeGeometryImplementation {
        std::vector<Untextured_vert>& vert_swap;
        Gpu_cache& gpu_cache;
        SimTK::SimbodyMatterSubsystem const& matter_subsys;
        SimTK::State const& state;

    public:
        constexpr Simbody_geometry_visitor(
            std::vector<Untextured_vert>& _vert_swap,
            Gpu_cache& _cache,
            SimTK::SimbodyMatterSubsystem const& _matter,
            SimTK::State const& _state) noexcept :

            SimTK::DecorativeGeometryImplementation{},

            vert_swap{_vert_swap},
            gpu_cache{_cache},
            matter_subsys{_matter},
            state{_state} {
        }

    private:
        // called when the implementation emits a mesh instance
        virtual void on_instance_created(Mesh_instance const&) = 0;

        template<typename... Args>
        void emplace_instance(Args&&... args) {
            on_instance_created(Mesh_instance{std::forward<Args>(args)...});
        }

        // implementation details
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
}
