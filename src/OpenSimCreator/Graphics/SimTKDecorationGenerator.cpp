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
    Vec3 GetScaleFactors(const SimTK::DecorativeGeometry& geom)
    {
        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i)
        {
            sf[i] = sf[i] <= 0.0 ? 1.0 : sf[i];
        }

        return ToVec3(sf);
    }

    // extracts RGBA color from geometry
    Color GetColor(const SimTK::DecorativeGeometry& geom)
    {
        const SimTK::Vec3& rgb = geom.getColor();

        auto ar = static_cast<float>(geom.getOpacity());
        ar = ar < 0.0f ? 1.0f : ar;

        return Color{ToVec3(rgb), ar};
    }

    SceneDecorationFlags GetFlags(const SimTK::DecorativeGeometry& geom)
    {
        SceneDecorationFlags rv = SceneDecorationFlag::None;

        switch (geom.getRepresentation()) {
        case SimTK::DecorativeGeometry::Hide:
            rv |= SceneDecorationFlag::NoDrawNormally;
            break;
        case SimTK::DecorativeGeometry::DrawWireframe:
            rv |= SceneDecorationFlag::DrawWireframeOverlay;
            rv |= SceneDecorationFlag::NoDrawNormally;
            break;
        default:
            rv |= SceneDecorationFlag::CastsShadows;
            break;
        }
        return rv;
    }

    // creates a geometry-to-ground transform for the given geometry
    Transform ToOscTransform(
        const SimTK::SimbodyMatterSubsystem& matter,
        const SimTK::State& state,
        const SimTK::DecorativeGeometry& g)
    {
        const SimTK::MobilizedBody& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
        const SimTK::Transform& body2ground = mobod.getBodyTransform(state);
        const SimTK::Transform& decoration2body = g.getTransform();

        return decompose_to_transform(body2ground * decoration2body).with_scale(GetScaleFactors(g));
    }

    size_t hash_of(const SimTK::Vec3& v)
    {
        return osc::hash_of(v[0], v[1], v[2]);
    }

    size_t hash_of(const SimTK::PolygonalMesh& mesh)
    {
        size_t hash = 0;

        // combine vertex data into hash
        const int numVerts = mesh.getNumVertices();
        hash = osc::hash_combine(hash, osc::hash_of(numVerts));
        for (int vert = 0; vert < numVerts; ++vert)
        {
            hash = hash_combine(hash, hash_of(mesh.getVertexPosition(vert)));
        }

        // combine face indices into mesh
        const int numFaces = mesh.getNumFaces();
        hash = osc::hash_combine(hash, osc::hash_of(numFaces));
        for (int face = 0; face < numFaces; ++face)
        {
            const int numVertsInFace = mesh.getNumVerticesForFace(face);
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
            const SimTK::SimbodyMatterSubsystem& matter,
            const SimTK::State& st,
            float fixupScaleFactor,
            const std::function<void(SceneDecoration&&)>& out) :

            m_MeshCache{meshCache},
            m_Matter{matter},
            m_State{st},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Consumer{out}
        {
        }

    private:
        Transform ToOscTransform(const SimTK::DecorativeGeometry& d) const
        {
            return ::ToOscTransform(m_Matter, m_State, d);
        }

        void implementPointGeometry(const SimTK::DecorativePoint&) final
        {
            [[maybe_unused]] static const bool s_ShownWarningOnce = []()
            {
                log_warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementLineGeometry(const SimTK::DecorativeLine& d) final
        {
            const Transform t = ToOscTransform(d);
            const Vec3 p1 = t * ToVec3(d.getPoint1());
            const Vec3 p2 = t * ToVec3(d.getPoint2());

            const float thickness = c_LineThickness * m_FixupScaleFactor;

            Transform cylinderXform = cylinder_to_line_segment_transform({p1, p2}, thickness);
            cylinderXform.scale *= t.scale;

            m_Consumer({
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = cylinderXform,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementBrickGeometry(const SimTK::DecorativeBrick& d) final
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

        void implementCylinderGeometry(const SimTK::DecorativeCylinder& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());
            const auto halfHeight = static_cast<float>(d.getHalfHeight());

            Transform t = ToOscTransform(d);
            t.scale *= Vec3{radius, halfHeight , radius};

            m_Consumer({
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementCircleGeometry(const SimTK::DecorativeCircle& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());

            Transform t = ToOscTransform(d);
            t.scale *= Vec3{radius, radius, 1.0f};

            m_Consumer({
                .mesh = m_MeshCache.circle_mesh(),
                .transform = t,
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementSphereGeometry(const SimTK::DecorativeSphere& d) final
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

        void implementEllipsoidGeometry(const SimTK::DecorativeEllipsoid& d) final
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

        void implementFrameGeometry(const SimTK::DecorativeFrame& d) final
        {
            const Transform t = ToOscTransform(d);

            // emit origin sphere
            {
                const float radius = 0.05f * c_FrameAxisLengthRescale * m_FixupScaleFactor;
                const Transform sphereXform = t.with_scale(radius);

                m_Consumer({
                    .mesh = m_MeshCache.sphere_mesh(),
                    .transform = sphereXform,
                    .color = Color::white(),
                    .flags = GetFlags(d),
                });
            }

            // emit leg cylinders
            const Vec3 axisLengths = t.scale * static_cast<float>(d.getAxisLength());
            const float legLen = c_FrameAxisLengthRescale * m_FixupScaleFactor;
            const float legThickness = c_FrameAxisThickness * m_FixupScaleFactor;
            const auto flags = GetFlags(d);
            for (int axis = 0; axis < 3; ++axis)
            {
                Vec3 direction = {0.0f, 0.0f, 0.0f};
                direction[axis] = 1.0f;

                const LineSegment line =
                {
                    t.position,
                    t.position + (legLen * axisLengths[axis] * transform_direction(t, direction))
                };
                const Transform legXform = cylinder_to_line_segment_transform(line, legThickness);

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

        void implementTextGeometry(const SimTK::DecorativeText&) final
        {
            [[maybe_unused]] static const bool s_ShownWarningOnce = []()
            {
                log_warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementMeshGeometry(const SimTK::DecorativeMesh& d) final
        {
            // the ID of an in-memory mesh is derived from the hash of its data
            //
            // (Simbody visualizer uses memory addresses, but this is invalid in
            //  OSC because there's a chance of memory re-use screwing with that
            //  caching mechanism)
            //
            // (and, yes, hash isn't equality, but it's closer than relying on memory
            //  addresses)
            const std::string id = std::to_string(hash_of(d.getMesh()));
            const auto meshLoaderFunc = [&d]() { return ToOscMesh(d.getMesh()); };

            m_Consumer({
                .mesh = m_MeshCache.get_mesh(id, meshLoaderFunc),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementMeshFileGeometry(const SimTK::DecorativeMeshFile& d) final
        {
            const std::string& path = d.getMeshFile();
            const auto meshLoader = [&d](){ return ToOscMesh(d.getMesh()); };

            m_Consumer({
                .mesh = m_MeshCache.get_mesh(path, meshLoader),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementArrowGeometry(const SimTK::DecorativeArrow& d) final
        {
            const Transform t = ToOscTransform(d);

            const Vec3 startBase = ToVec3(d.getStartPoint());
            const Vec3 endBase = ToVec3(d.getEndPoint());

            const Vec3 start = transform_point(t, startBase);
            const Vec3 end = transform_point(t, endBase);

            const Vec3 direction = normalize(end - start);

            const Vec3 neckStart = start;
            const Vec3 neckEnd = end - (m_FixupScaleFactor * static_cast<float>(d.getTipLength()) * direction);
            const Vec3 headStart = neckEnd;
            const Vec3 headEnd = end;

            const float neck_thickness = m_FixupScaleFactor * static_cast<float>(d.getLineThickness());
            const float head_thickness = 1.75f * neck_thickness;

            const Color color = GetColor(d);
            const auto flags = GetFlags(d);

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

        void implementTorusGeometry(const SimTK::DecorativeTorus& d) final
        {
            const auto tube_center_radius = static_cast<float>(d.getTorusRadius());
            const auto tube_radius = static_cast<float>(d.getTubeRadius());

            m_Consumer({
                .mesh = m_MeshCache.torus_mesh(tube_center_radius, tube_radius),
                .transform = ToOscTransform(d),
                .color = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementConeGeometry(const SimTK::DecorativeCone& d) final
        {
            const Transform t = ToOscTransform(d);

            const Vec3 posBase = ToVec3(d.getOrigin());
            const Vec3 posDir = ToVec3(d.getDirection());

            const Vec3 pos = transform_point(t, posBase);
            const Vec3 direction = transform_direction(t, posDir);

            const auto radius = static_cast<float>(d.getBaseRadius());
            const auto height = static_cast<float>(d.getHeight());

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
        const SimTK::SimbodyMatterSubsystem& m_Matter;
        const SimTK::State& m_State;
        float m_FixupScaleFactor;
        const std::function<void(SceneDecoration&&)>& m_Consumer;
    };
}

void osc::GenerateDecorations(
    SceneCache& meshCache,
    const SimTK::SimbodyMatterSubsystem& matter,
    const SimTK::State& state,
    const SimTK::DecorativeGeometry& geom,
    float fixupScaleFactor,
    const std::function<void(SceneDecoration&&)>& out)
{
    GeometryImpl impl{meshCache, matter, state, fixupScaleFactor, out};
    geom.implementGeometry(impl);
}
