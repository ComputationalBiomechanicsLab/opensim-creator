#pragma once

#include <SimTKcommon.h>
#include <glm/mat4x4.hpp>

#include <filesystem>
#include <glm/vec3.hpp>

namespace osc {
    struct Untextured_mesh;
    struct GPU_storage;
    struct Mesh_instance;
}

namespace SimTK {
    class SimbodyMatterSubsystem;
}

namespace osc {

    // convert 3 packed floats to a SimTK::Vec3
    [[nodiscard]] inline SimTK::Vec3 stk_vec3_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }

    // convert a glm::vec3 to a SimTK::Vec3
    [[nodiscard]] inline SimTK::Vec3 stk_vec3_from(glm::vec3 const& v) noexcept {
        return {
            static_cast<double>(v.x),
            static_cast<double>(v.y),
            static_cast<double>(v.z),
        };
    }

    // convert 3 packed floats to a SimTK::Inertia
    [[nodiscard]] inline SimTK::Inertia stk_inertia_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }

    // convert a SimTK::Transform to a glm::mat4
    [[nodiscard]] glm::mat4 to_mat4(SimTK::Transform const& t) noexcept;

    // convert a glm::mat4 to a SimTK::Transform
    [[nodiscard]] SimTK::Transform to_transform(glm::mat4 const&) noexcept;

    // load a mesh file, using the SimTK backend, into an OSC-friendly Untextured_mesh
    //
    // the mesh is cleared by the implementation before appending the loaded verts
    void load_mesh_file_with_simtk_backend(std::filesystem::path const&, Untextured_mesh&);

    // SimTK::DecorativeGeometryImplementation that can emit instanced geometry
    class Simbody_geometry_visitor : public SimTK::DecorativeGeometryImplementation {
        Untextured_mesh& mesh_swap;
        GPU_storage& gpu_cache;
        SimTK::SimbodyMatterSubsystem const& matter_subsys;
        SimTK::State const& state;

    public:
        constexpr Simbody_geometry_visitor(
            Untextured_mesh& _mesh_swap,
            GPU_storage& _cache,
            SimTK::SimbodyMatterSubsystem const& _matter,
            SimTK::State const& _state) noexcept :

            SimTK::DecorativeGeometryImplementation{},

            mesh_swap{_mesh_swap},
            gpu_cache{_cache},
            matter_subsys{_matter},
            state{_state} {
        }

    private:
        // called whenever the implementation emits a mesh instance
        virtual void on_instance_created(Mesh_instance const& mi) = 0;

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

    // C++ lambda implementation of the above
    template<typename OnInstanceCreatedCallback>
    class Lambda_geometry_visitor final : public Simbody_geometry_visitor {
        OnInstanceCreatedCallback callback;

    public:
        template<typename... BaseCtorArgs>
        Lambda_geometry_visitor(OnInstanceCreatedCallback _callback, BaseCtorArgs&&... args) :
            Simbody_geometry_visitor{std::forward<BaseCtorArgs>(args)...},
            callback(std::move(_callback)) {
        }

    private:
        void on_instance_created(Mesh_instance const& mi) override {
            callback(mi);
        }
    };
}
