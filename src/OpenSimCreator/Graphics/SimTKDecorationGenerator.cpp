#include "SimTKDecorationGenerator.h"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/HashHelpers.h>
#include <simbody/internal/common.h>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>
#include <SimTKcommon/internal/State.h>

#include <cstddef>
#include <filesystem>

using namespace osc;

// helper functions
namespace
{
    inline constexpr float c_LineThickness = 0.005f;
    inline constexpr float c_FrameAxisLengthRescale = 0.25f;
    inline constexpr float c_FrameAxisThickness = 0.0025f;

    // extracts scale factors from geometry
    Vec3 GetScaleFactors(SimTK::DecorativeGeometry const& geom)
    {
        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i)
        {
            sf[i] = sf[i] <= 0.0 ? 1.0 : sf[i];
        }

        return ToVec3(sf);
    }

    // extracts RGBA color from geometry
    Color GetColor(SimTK::DecorativeGeometry const& geom)
    {
        SimTK::Vec3 const& rgb = geom.getColor();

        auto ar = static_cast<float>(geom.getOpacity());
        ar = ar < 0.0f ? 1.0f : ar;

        return Color{ToVec3(rgb), ar};
    }

    SceneDecorationFlags GetFlags(SimTK::DecorativeGeometry const& geom)
    {
        SceneDecorationFlags rv = SceneDecorationFlags::None;
        switch (geom.getRepresentation()) {
        case SimTK::DecorativeGeometry::Hide:
            rv |= SceneDecorationFlags::NoDrawNormally;
            break;
        case SimTK::DecorativeGeometry::DrawWireframe:
            rv |= SceneDecorationFlags::WireframeOverlay | SceneDecorationFlags::NoDrawNormally;
            break;
        default:
            rv |= SceneDecorationFlags::CastsShadows;
            break;
        }
        return rv;
    }

    // creates a geometry-to-ground transform for the given geometry
    Transform ToOscTransform(
        SimTK::SimbodyMatterSubsystem const& matter,
        SimTK::State const& state,
        SimTK::DecorativeGeometry const& g)
    {
        SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
        SimTK::Transform const& body2ground = mobod.getBodyTransform(state);
        SimTK::Transform const& decoration2body = g.getTransform();

        return decompose_to_transform(body2ground * decoration2body).with_scale(GetScaleFactors(g));
    }

    size_t hash_of(SimTK::Vec3 const& v)
    {
        return osc::hash_of(v[0], v[1], v[2]);
    }

    size_t hash_of(SimTK::PolygonalMesh const& mesh)
    {
        size_t hash = 0;

        // combine vertex data into hash
        int const numVerts = mesh.getNumVertices();
        hash = osc::hash_combine(hash, osc::hash_of(numVerts));
        for (int vert = 0; vert < numVerts; ++vert)
        {
            hash = hash_combine(hash, hash_of(mesh.getVertexPosition(vert)));
        }

        // combine face indices into mesh
        int const numFaces = mesh.getNumFaces();
        hash = osc::hash_combine(hash, osc::hash_of(numFaces));
        for (int face = 0; face < numFaces; ++face)
        {
            int const numVertsInFace = mesh.getNumVerticesForFace(face);
            for (int faceVert = 0; faceVert < numVertsInFace; ++faceVert)
            {
                hash = osc::hash_combine(hash, osc::hash_of(mesh.getFaceVertex(face, faceVert)));
            }
        }

        return hash;
    }

    // an implementation of SimTK::DecorativeGeometryImplementation that emits generic
    // triangle-mesh-based SystemDecorations that can be consumed by the rest of the UI
    class GeometryImpl final : public SimTK::DecorativeGeometryImplementation {
    public:
        GeometryImpl(
            SceneCache& meshCache,
            SimTK::SimbodyMatterSubsystem const& matter,
            SimTK::State const& st,
            float fixupScaleFactor,
            std::function<void(SceneDecoration&&)> const& out) :

            m_MeshCache{meshCache},
            m_Matter{matter},
            m_State{st},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Consumer{out}
        {
        }

    private:
        Transform ToOscTransform(SimTK::DecorativeGeometry const& d) const
        {
            return ::ToOscTransform(m_Matter, m_State, d);
        }

        void implementPointGeometry(SimTK::DecorativePoint const&) final
        {
            [[maybe_unused]] static bool const s_ShownWarningOnce = []()
            {
                log_warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementLineGeometry(SimTK::DecorativeLine const& d) final
        {
            Transform const t = ToOscTransform(d);
            Vec3 const p1 = t * ToVec3(d.getPoint1());
            Vec3 const p2 = t * ToVec3(d.getPoint2());

            float const thickness = c_LineThickness * m_FixupScaleFactor;

            Transform cylinderXform = cylinder_to_line_segment_transform({p1, p2}, thickness);
            cylinderXform.scale *= t.scale;

            m_Consumer({
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = cylinderXform,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementBrickGeometry(SimTK::DecorativeBrick const& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= ToVec3(d.getHalfLengths());

            m_Consumer({
                .mesh = m_MeshCache.brick_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementCylinderGeometry(SimTK::DecorativeCylinder const& d) final
        {
            auto const radius = static_cast<float>(d.getRadius());
            auto const halfHeight = static_cast<float>(d.getHalfHeight());

            Transform t = ToOscTransform(d);
            t.scale *= Vec3{radius, halfHeight , radius};

            m_Consumer({
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementCircleGeometry(SimTK::DecorativeCircle const& d) final
        {
            auto const radius = static_cast<float>(d.getRadius());

            Transform t = ToOscTransform(d);
            t.scale *= Vec3{radius, radius, 1.0f};

            m_Consumer({
                .mesh = m_MeshCache.circle_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementSphereGeometry(SimTK::DecorativeSphere const& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= m_FixupScaleFactor * static_cast<float>(d.getRadius());

            m_Consumer({
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= ToVec3(d.getRadii());

            m_Consumer({
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementFrameGeometry(SimTK::DecorativeFrame const& d) final
        {
            Transform const t = ToOscTransform(d);

            // emit origin sphere
            {
                float const radius = 0.05f * c_FrameAxisLengthRescale * m_FixupScaleFactor;
                Transform const sphereXform = t.with_scale(radius);

                m_Consumer({
                    .mesh = m_MeshCache.sphere_mesh(),
                    .transform = sphereXform,
                    .color = Color::white(),
                    .flags = GetFlags(d),
                });
            }

            // emit leg cylinders
            Vec3 const axisLengths = t.scale * static_cast<float>(d.getAxisLength());
            float const legLen = c_FrameAxisLengthRescale * m_FixupScaleFactor;
            float const legThickness = c_FrameAxisThickness * m_FixupScaleFactor;
            auto const flags = GetFlags(d);
            for (int axis = 0; axis < 3; ++axis)
            {
                Vec3 direction = {0.0f, 0.0f, 0.0f};
                direction[axis] = 1.0f;

                LineSegment const line =
                {
                    t.position,
                    t.position + (legLen * axisLengths[axis] * transform_direction(t, direction))
                };
                Transform const legXform = cylinder_to_line_segment_transform(line, legThickness);

                Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[axis] = 1.0f;

                m_Consumer({
                    .mesh = m_MeshCache.cylinder_mesh(),
                    .transform = legXform,
                    .color = color,
                    .flags = flags,
                });
            }
        }

        void implementTextGeometry(SimTK::DecorativeText const&) final
        {
            [[maybe_unused]] static bool const s_ShownWarningOnce = []()
            {
                log_warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementMeshGeometry(SimTK::DecorativeMesh const& d) final
        {
            // the ID of an in-memory mesh is derived from the hash of its data
            //
            // (Simbody visualizer uses memory addresses, but this is invalid in
            //  OSC because there's a chance of memory re-use screwing with that
            //  caching mechanism)
            //
            // (and, yes, hash isn't equality, but it's closer than relying on memory
            //  addresses)
            std::string const id = std::to_string(hash_of(d.getMesh()));
            auto const meshLoaderFunc = [&d]() { return ToOscMesh(d.getMesh()); };

            m_Consumer({
                .mesh = m_MeshCache.get_mesh(id, meshLoaderFunc),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementMeshFileGeometry(SimTK::DecorativeMeshFile const& d) final
        {
            std::string const& path = d.getMeshFile();
            auto const meshLoader = [&d](){ return ToOscMesh(d.getMesh()); };

            m_Consumer({
                .mesh = m_MeshCache.get_mesh(path, meshLoader),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementArrowGeometry(SimTK::DecorativeArrow const& d) final
        {
            Transform const t = ToOscTransform(d);

            Vec3 const startBase = ToVec3(d.getStartPoint());
            Vec3 const endBase = ToVec3(d.getEndPoint());

            Vec3 const start = transform_point(t, startBase);
            Vec3 const end = transform_point(t, endBase);

            Vec3 const direction = normalize(end - start);

            Vec3 const neckStart = start;
            Vec3 const neckEnd = end - (m_FixupScaleFactor * static_cast<float>(d.getTipLength()) * direction);
            Vec3 const headStart = neckEnd;
            Vec3 const headEnd = end;

            float const neck_thickness = m_FixupScaleFactor * static_cast<float>(d.getLineThickness());
            float const head_thickness = 1.75f * neck_thickness;

            Color const color = GetColor(d);
            auto const flags = GetFlags(d);

            // emit neck cylinder
            m_Consumer({
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = cylinder_to_line_segment_transform({neckStart, neckEnd}, neck_thickness),
                .color = color,
                .flags = flags,
            });

            // emit head cone
            m_Consumer({
                .mesh = m_MeshCache.cone_mesh(),
                .transform = cylinder_to_line_segment_transform({headStart, headEnd}, head_thickness),
                .color = color,
                .flags = flags,
            });
        }

        void implementTorusGeometry(SimTK::DecorativeTorus const& d) final
        {
            auto const tube_center_radius = static_cast<float>(d.getTorusRadius());
            auto const tube_radius = static_cast<float>(d.getTubeRadius());

            m_Consumer({
                .mesh = m_MeshCache.torus_mesh(tube_center_radius, tube_radius),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementConeGeometry(SimTK::DecorativeCone const& d) final
        {
            Transform const t = ToOscTransform(d);

            Vec3 const posBase = ToVec3(d.getOrigin());
            Vec3 const posDir = ToVec3(d.getDirection());

            Vec3 const pos = transform_point(t, posBase);
            Vec3 const direction = transform_direction(t, posDir);

            auto const radius = static_cast<float>(d.getBaseRadius());
            auto const height = static_cast<float>(d.getHeight());

            Transform coneXform = cylinder_to_line_segment_transform({pos, pos + height*direction}, radius);
            coneXform.scale *= t.scale;

            m_Consumer({
                .mesh = m_MeshCache.cone_mesh(),
                .transform = coneXform,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        SceneCache& m_MeshCache;
        SimTK::SimbodyMatterSubsystem const& m_Matter;
        SimTK::State const& m_State;
        float m_FixupScaleFactor;
        std::function<void(SceneDecoration&&)> const& m_Consumer;
    };
}

void osc::GenerateDecorations(
    SceneCache& meshCache,
    SimTK::SimbodyMatterSubsystem const& matter,
    SimTK::State const& state,
    SimTK::DecorativeGeometry const& geom,
    float fixupScaleFactor,
    std::function<void(SceneDecoration&&)> const& out)
{
    GeometryImpl impl{meshCache, matter, state, fixupScaleFactor, out};
    geom.implementGeometry(impl);
}
