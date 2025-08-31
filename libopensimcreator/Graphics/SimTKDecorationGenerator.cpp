#include "SimTKDecorationGenerator.h"

#include <libopensimcreator/Graphics/SimTKMeshLoader.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/LineSegment.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/HashHelpers.h>
#include <simbody/internal/common.h>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>
#include <SimTKcommon/internal/State.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <ranges>

using namespace osc;

// helper functions
namespace
{
    inline constexpr float c_LineThickness = 0.005f;
    inline constexpr float c_FrameAxisLengthRescale = 0.25f;
    inline constexpr float c_FrameAxisThickness = 0.0025f;

    // extracts scale factors from geometry
    Vector3 GetScaleFactors(const SimTK::DecorativeGeometry& geom)
    {
        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i) {
            // filter out NaNs, but keep negative values, because some
            // users use negative scales to mimic mirror imaging (#974)
            sf[i] = not std::isnan(sf[i]) ? sf[i] : 0.0;
        }

        return to<Vector3>(sf);
    }

    float GetOpacity(const SimTK::DecorativeGeometry& geometry)
    {
        const auto rv = static_cast<float>(geometry.getOpacity());
        return rv >= 0.0f ? rv : 1.0f;
    }

    // returns the color of `geometry`, with any defaults saturated to `1.0f`
    Color GetColor(const SimTK::DecorativeGeometry& geometry)
    {
        auto rgb = to<Vector3>(geometry.getColor());

        // Simbody uses `-1` to mean "use default`. We use a default of `1.0f`
        // whenever this, or a NaN, occurs.
        for (auto& component : rgb) {
            component = component >= 0.0f ? component : 1.0f;
        }
        return Color{rgb, GetOpacity(geometry)};
    }

    // Returns `true` if `geometry` has a defaulted color
    bool IsDefaultColor(const SimTK::DecorativeGeometry& geometry)
    {
        return geometry.getColor() == SimTK::Vec3{-1.0, -1.0, -1.0};
    }

    SceneDecorationFlags GetFlags(const SimTK::DecorativeGeometry& geom)
    {
        switch (geom.getRepresentation()) {
        case SimTK::DecorativeGeometry::DrawWireframe:
            return SceneDecorationFlag::OnlyWireframe;
        case SimTK::DecorativeGeometry::Hide:
            return SceneDecorationFlag::Hidden;
        default:
            return SceneDecorationFlag::Default;
        }
    }

    // creates a geometry-to-ground transform for the given geometry
    Transform ToOscTransformWithoutScaling(
        const SimTK::SimbodyMatterSubsystem& matter,
        const SimTK::State& state,
        const SimTK::DecorativeGeometry& g)
    {
        const SimTK::MobilizedBody& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
        const SimTK::Transform& body2ground = mobod.getBodyTransform(state);
        const SimTK::Transform& decoration2body = g.getTransform();

        return to<Transform>(body2ground * decoration2body);
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
        Transform ToOscTransformWithoutScaling(const SimTK::DecorativeGeometry& d) const
        {
            return ::ToOscTransformWithoutScaling(m_Matter, m_State, d);
        }

        Transform ToOscTransform(const SimTK::DecorativeGeometry& d) const
        {
            return ToOscTransformWithoutScaling(d).with_scale(GetScaleFactors(d));
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
            const Vector3 p1 = t * to<Vector3>(d.getPoint1());
            const Vector3 p2 = t * to<Vector3>(d.getPoint2());

            const float thickness = c_LineThickness * m_FixupScaleFactor;

            Transform cylinderXform = cylinder_to_line_segment_transform({p1, p2}, thickness);
            cylinderXform.scale *= t.scale;

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = cylinderXform,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementBrickGeometry(const SimTK::DecorativeBrick& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= to<Vector3>(d.getHalfLengths());

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.brick_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementCylinderGeometry(const SimTK::DecorativeCylinder& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());
            const auto halfHeight = static_cast<float>(d.getHalfHeight());

            Transform t = ToOscTransform(d);
            t.scale *= Vector3{radius, halfHeight , radius};

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementCircleGeometry(const SimTK::DecorativeCircle& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());

            Transform t = ToOscTransform(d);
            t.scale *= Vector3{radius, radius, 1.0f};

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.circle_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementSphereGeometry(const SimTK::DecorativeSphere& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= m_FixupScaleFactor * static_cast<float>(d.getRadius());

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementEllipsoidGeometry(const SimTK::DecorativeEllipsoid& d) final
        {
            Transform t = ToOscTransform(d);
            t.scale *= to<Vector3>(d.getRadii());

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementFrameGeometry(const SimTK::DecorativeFrame& d) final
        {
            const Transform t = ToOscTransform(d);

            // if the calling code explicitly sets the color of a frame as non-white, then
            // that override should be obeyed, rather than using OSC's custom coloring
            // scheme (#985).
            const std::optional<Color> colorOverride = IsDefaultColor(d)  or d.getColor() == SimTK::Vec3{1.0, 1.0, 1.0} ?
                std::optional<Color>{} :
                GetColor(d);

            // emit origin sphere
            {
                const float radius = 0.05f * c_FrameAxisLengthRescale * m_FixupScaleFactor;
                const Transform sphereXform = t.with_scale(radius);

                m_Consumer(SceneDecoration{
                    .mesh = m_MeshCache.sphere_mesh(),
                    .transform = sphereXform,
                    .shading = colorOverride ? *colorOverride : Color::white(),
                    .flags = GetFlags(d),
                });
            }

            // emit leg cylinders
            const Vector3 axisLengths = t.scale * static_cast<float>(d.getAxisLength());
            const float legLen = c_FrameAxisLengthRescale * m_FixupScaleFactor;
            const float legThickness = c_FrameAxisThickness * m_FixupScaleFactor;
            const auto flags = GetFlags(d);
            for (int axis = 0; axis < 3; ++axis) {
                Vector3 direction = {0.0f, 0.0f, 0.0f};
                direction[axis] = 1.0f;

                const LineSegment lineSegment = {
                    t.translation,
                    t.translation + (legLen * axisLengths[axis] * transform_direction(t, direction))
                };
                const Transform legXform = cylinder_to_line_segment_transform(lineSegment, legThickness);

                Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[axis] = 1.0f;

                m_Consumer(SceneDecoration{
                    .mesh = m_MeshCache.cylinder_mesh(),
                    .transform = legXform,
                    .shading = colorOverride ? *colorOverride : color,
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

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.get_mesh(id, meshLoaderFunc),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementMeshFileGeometry(const SimTK::DecorativeMeshFile& d) final
        {
            const std::string& path = d.getMeshFile();
            const auto meshLoader = [&d](){ return ToOscMesh(d.getMesh()); };

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.get_mesh(path, meshLoader),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementArrowGeometry(const SimTK::DecorativeArrow& d) final
        {
            const Transform t = ToOscTransformWithoutScaling(d);
            const ArrowProperties p = {
                .start = t * to<Vector3>(d.getStartPoint()),
                .end = t * to<Vector3>(d.getEndPoint()),
                .tip_length = static_cast<float>(d.getTipLength()),
                .neck_thickness = m_FixupScaleFactor * static_cast<float>(d.getLineThickness()),
                .head_thickness = 1.75f * m_FixupScaleFactor * static_cast<float>(d.getLineThickness()),
                .color = GetColor(d),
                .decoration_flags = GetFlags(d),
            };
            draw_arrow(m_MeshCache, p, m_Consumer);
        }

        void implementTorusGeometry(const SimTK::DecorativeTorus& d) final
        {
            const auto tube_center_radius = static_cast<float>(d.getTorusRadius());
            const auto tube_radius = static_cast<float>(d.getTubeRadius());

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.torus_mesh(tube_center_radius, tube_radius),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d),
            });
        }

        void implementConeGeometry(const SimTK::DecorativeCone& d) final
        {
            const Transform t = ToOscTransform(d);

            auto posBase = to<Vector3>(d.getOrigin());
            auto posDir = to<Vector3>(d.getDirection());

            const Vector3 pos = transform_point(t, posBase);
            const Vector3 direction = transform_direction(t, posDir);

            const auto radius = static_cast<float>(d.getBaseRadius());
            const auto height = static_cast<float>(d.getHeight());

            Transform coneXform = cylinder_to_line_segment_transform({pos, pos + height*direction}, radius);
            coneXform.scale *= t.scale;

            m_Consumer(SceneDecoration{
                .mesh = m_MeshCache.cone_mesh(),
                .transform = coneXform,
                .shading = GetColor(d),
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
