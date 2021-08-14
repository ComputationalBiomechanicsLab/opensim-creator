#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <SimTKcommon.h>

#include <string>
#include <iosfwd>

namespace SimTK {
    class SimbodyMatterSubsystem;
    class State;
}

namespace osc {
    struct Simbody_geometry final {

        struct Sphere final {
            glm::vec4 rgba;
            glm::vec3 pos;
            float radius;
        };

        struct Line final {
            glm::vec4 rgba;
            glm::vec3 p1;
            glm::vec3 p2;
        };

        // assumed to be based on a cylinder between -1 and +1 in Y
        // with radius of 1
        struct Cylinder final {
            glm::vec4 rgba;
            glm::mat4 model_mtx;
        };

        // assumed to be based on a cube that is -1 to +1 in each axis
        struct Brick final {
            glm::vec4 rgba;
            glm::mat4 model_mtx;
        };

        struct MeshFile final {
            std::string const* path;  // lives only as long as the emission step
            glm::vec4 rgba;
            glm::mat4 model_mtx;
        };

        struct Frame final {
            glm::vec3 pos;
            glm::vec3 axis_lengths;
            glm::mat3 rotation;
        };

        struct Ellipsoid final {
            glm::vec4 rgba;
            glm::mat4 model_mtx;
        };

        struct Cone final {
            glm::vec4 rgba;
            glm::vec3 pos;
            glm::vec3 direction;
            float base_radius;
            float height;
        };

        struct Arrow final {
            glm::vec4 rgba;
            glm::vec3 p1;
            glm::vec3 p2;
        };

        // possible data held (check the tag)
        union {
            Sphere sphere;
            Line line;
            Cylinder cylinder;
            Brick brick;
            MeshFile meshfile;
            Frame frame;
            Ellipsoid ellipsoid;
            Cone cone;
            Arrow arrow;
        };

        // tag
        enum class Type {
            Sphere,
            Line,
            Cylinder,
            Brick,
            MeshFile,
            Frame,
            Ellipsoid,
            Cone,
            Arrow,
        } geom_type;
    };

    std::ostream& operator<<(std::ostream&, Simbody_geometry::Sphere const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Line const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Cylinder const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Brick const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::MeshFile const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Frame const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Ellipsoid const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Cone const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry::Arrow const&);
    std::ostream& operator<<(std::ostream&, Simbody_geometry const&);

    class Geometry_generator : public SimTK::DecorativeGeometryImplementation {
        SimTK::SimbodyMatterSubsystem const& m_Matter;
        SimTK::State const& m_St;

    public:
        Geometry_generator(SimTK::SimbodyMatterSubsystem const&, SimTK::State const&);

    private:
        virtual void on_emit(Simbody_geometry const&) = 0;

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

    template<typename Consumer>
    class Geometry_generator_lambda final : public Geometry_generator {
        Consumer m_Consumer;
    public:
        Geometry_generator_lambda(
                SimTK::SimbodyMatterSubsystem const& matter,
                SimTK::State const& st,
                Consumer consumer) :
            Geometry_generator{matter, st},
            m_Consumer{consumer} {
        }
    private:
        void on_emit(Simbody_geometry const& g) override {
            m_Consumer(g);
        }
    };
}
