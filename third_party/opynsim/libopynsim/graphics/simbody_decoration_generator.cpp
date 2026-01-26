#include "simbody_decoration_generator.h"

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/maths/line_segment.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/hash_helpers.h>
#include <simbody/internal/common.h>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

using namespace opyn;

// helper functions
namespace
{
    inline constexpr float c_LineThickness = 0.005f;
    inline constexpr float c_FrameAxisLengthRescale = 0.25f;
    inline constexpr float c_FrameAxisThickness = 0.0025f;

    // extracts scale factors from geometry
    osc::Vector3 GetScaleFactors(const SimTK::DecorativeGeometry& geom)
    {
        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i) {
            // filter out NaNs, but keep negative values, because some
            // users use negative scales to mimic mirror imaging (#974)
            sf[i] = not std::isnan(sf[i]) ? sf[i] : 0.0;
        }

        return osc::to<osc::Vector3>(sf);
    }

    float GetOpacity(const SimTK::DecorativeGeometry& geometry)
    {
        const auto rv = static_cast<float>(geometry.getOpacity());
        return rv >= 0.0f ? rv : 1.0f;
    }

    // returns the color of `geometry`, with any defaults saturated to `1.0f`
    osc::Color GetColor(const SimTK::DecorativeGeometry& geometry)
    {
        auto rgb = osc::to<osc::Vector3>(geometry.getColor());

        // Simbody uses `-1` to mean "use default`. We use a default of `1.0f`
        // whenever this, or a NaN, occurs.
        for (auto& component : rgb) {
            component = component >= 0.0f ? component : 1.0f;
        }
        return osc::Color{rgb, GetOpacity(geometry)};
    }

    // Returns `true` if `geometry` has a defaulted color
    bool IsDefaultColor(const SimTK::DecorativeGeometry& geometry)
    {
        return geometry.getColor() == SimTK::Vec3{-1.0, -1.0, -1.0};
    }

    osc::SceneDecorationFlags GetFlags(const SimTK::DecorativeGeometry& geom)
    {
        switch (geom.getRepresentation()) {
        case SimTK::DecorativeGeometry::DrawWireframe:
            return osc::SceneDecorationFlag::OnlyWireframe;
        case SimTK::DecorativeGeometry::Hide:
            return osc::SceneDecorationFlag::Hidden;
        default:
            return osc::SceneDecorationFlag::Default;
        }
    }

    // creates a geometry-to-ground transform for the given geometry
    osc::Transform ToOscTransformWithoutScaling(
        const SimTK::SimbodyMatterSubsystem& matter,
        const SimTK::State& state,
        const SimTK::DecorativeGeometry& g)
    {
        const SimTK::MobilizedBody& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
        const SimTK::Transform& body2ground = mobod.getBodyTransform(state);
        const SimTK::Transform& decoration2body = g.getTransform();

        return osc::to<osc::Transform>(body2ground * decoration2body);
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
            hash = osc::hash_combine(hash, hash_of(mesh.getVertexPosition(vert)));
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
            osc::SceneCache& meshCache,
            const SimTK::SimbodyMatterSubsystem& matter,
            const SimTK::State& st,
            float fixupScaleFactor,
            const std::function<void(osc::SceneDecoration&&)>& out) :

            m_MeshCache{meshCache},
            m_Matter{matter},
            m_State{st},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Consumer{out}
        {
        }

    private:
        osc::Transform ToOscTransformWithoutScaling(const SimTK::DecorativeGeometry& d) const
        {
            return ::ToOscTransformWithoutScaling(m_Matter, m_State, d);
        }

        osc::Transform ToOscTransform(const SimTK::DecorativeGeometry& d) const
        {
            return ToOscTransformWithoutScaling(d).with_scale(GetScaleFactors(d));
        }

        void implementPointGeometry(const SimTK::DecorativePoint&) final
        {
            [[maybe_unused]] static const bool s_ShownWarningOnce = []()
            {
                osc::log_warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementLineGeometry(const SimTK::DecorativeLine& d) final
        {
            const osc::Transform t = ToOscTransform(d);
            const osc::Vector3 p1 = t * osc::to<osc::Vector3>(d.getPoint1());
            const osc::Vector3 p2 = t * osc::to<osc::Vector3>(d.getPoint2());

            const float thickness = c_LineThickness * m_FixupScaleFactor;

            osc::Transform cylinderXform = osc::cylinder_to_line_segment_transform({p1, p2}, thickness);
            cylinderXform.scale *= t.scale;

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = cylinderXform,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementBrickGeometry(const SimTK::DecorativeBrick& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::to<osc::Vector3>(d.getHalfLengths());

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.brick_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementCylinderGeometry(const SimTK::DecorativeCylinder& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());
            const auto halfHeight = static_cast<float>(d.getHalfHeight());

            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::Vector3{radius, halfHeight , radius};

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.cylinder_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementCircleGeometry(const SimTK::DecorativeCircle& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());

            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::Vector3{radius, radius, 1.0f};

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.circle_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementSphereGeometry(const SimTK::DecorativeSphere& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= m_FixupScaleFactor * static_cast<float>(d.getRadius());

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementEllipsoidGeometry(const SimTK::DecorativeEllipsoid& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::to<osc::Vector3>(d.getRadii());

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.sphere_mesh(),
                .transform = t,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementFrameGeometry(const SimTK::DecorativeFrame& d) final
        {
            const osc::Transform t = ToOscTransform(d);

            // if the calling code explicitly sets the color of a frame as non-white, then
            // that override should be obeyed, rather than using OSC's custom coloring
            // scheme (#985).
            const std::optional<osc::Color> colorOverride = IsDefaultColor(d)  or d.getColor() == SimTK::Vec3{1.0, 1.0, 1.0} ?
                std::optional<osc::Color>{} :
                GetColor(d);

            // emit origin sphere
            {
                const float radius = 0.05f * c_FrameAxisLengthRescale * m_FixupScaleFactor;
                const osc::Transform sphereXform = t.with_scale(radius);

                m_Consumer(osc::SceneDecoration{
                    .mesh = m_MeshCache.sphere_mesh(),
                    .transform = sphereXform,
                    .shading = colorOverride ? *colorOverride : osc::Color::white(),
                    .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
                });
            }

            // emit leg cylinders
            const osc::Vector3 axisLengths = t.scale * static_cast<float>(d.getAxisLength());
            const float legLen = c_FrameAxisLengthRescale * m_FixupScaleFactor;
            const float legThickness = c_FrameAxisThickness * m_FixupScaleFactor;
            const auto flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull;
            for (int axis = 0; axis < 3; ++axis) {
                osc::Vector3 direction = {0.0f, 0.0f, 0.0f};
                direction[axis] = 1.0f;

                const osc::LineSegment lineSegment = {
                    t.translation,
                    t.translation + (legLen * axisLengths[axis] * transform_direction(t, direction))
                };
                const osc::Transform legXform = cylinder_to_line_segment_transform(lineSegment, legThickness);

                osc::Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[axis] = 1.0f;

                m_Consumer(osc::SceneDecoration{
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
                osc::log_warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
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

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.get_mesh(id, meshLoaderFunc),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d),  // no `SceneDecorationFlag::CanBackfaceCull`, because mesh data might be invalid (#318, #168)
            });
        }

        void implementMeshFileGeometry(const SimTK::DecorativeMeshFile& d) final
        {
            const std::string& path = d.getMeshFile();
            const auto meshLoader = [&d](){ return ToOscMesh(d.getMesh()); };

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.get_mesh(path, meshLoader),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d),  // no `SceneDecorationFlag::CanBackfaceCull`, because mesh data might be invalid (#318, #168)
            });
        }

        void implementArrowGeometry(const SimTK::DecorativeArrow& d) final
        {
            const osc::Transform t = ToOscTransformWithoutScaling(d);
            const osc::ArrowProperties p = {
                .start = t * osc::to<osc::Vector3>(d.getStartPoint()),
                .end = t * osc::to<osc::Vector3>(d.getEndPoint()),
                .tip_length = static_cast<float>(d.getTipLength()),
                .neck_thickness = m_FixupScaleFactor * static_cast<float>(d.getLineThickness()),
                .head_thickness = 1.75f * m_FixupScaleFactor * static_cast<float>(d.getLineThickness()),
                .color = GetColor(d),
                .decoration_flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            };
            draw_arrow(m_MeshCache, p, m_Consumer);
        }

        void implementTorusGeometry(const SimTK::DecorativeTorus& d) final
        {
            const auto tube_center_radius = static_cast<float>(d.getTorusRadius());
            const auto tube_radius = static_cast<float>(d.getTubeRadius());

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.torus_mesh(tube_center_radius, tube_radius),
                .transform = ToOscTransform(d),
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementConeGeometry(const SimTK::DecorativeCone& d) final
        {
            const osc::Transform t = ToOscTransform(d);

            auto posBase = osc::to<osc::Vector3>(d.getOrigin());
            auto posDir = osc::to<osc::Vector3>(d.getDirection());

            const osc::Vector3 pos = transform_point(t, posBase);
            const osc::Vector3 direction = transform_direction(t, posDir);

            const auto radius = static_cast<float>(d.getBaseRadius());
            const auto height = static_cast<float>(d.getHeight());

            osc::Transform coneXform = osc::cylinder_to_line_segment_transform({pos, pos + height*direction}, radius);
            coneXform.scale *= t.scale;

            m_Consumer(osc::SceneDecoration{
                .mesh = m_MeshCache.cone_mesh(),
                .transform = coneXform,
                .shading = GetColor(d),
                .flags = GetFlags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        osc::SceneCache& m_MeshCache;
        const SimTK::SimbodyMatterSubsystem& m_Matter;
        const SimTK::State& m_State;
        float m_FixupScaleFactor;
        const std::function<void(osc::SceneDecoration&&)>& m_Consumer;
    };
}

void opyn::GenerateDecorations(
    osc::SceneCache& meshCache,
    const SimTK::SimbodyMatterSubsystem& matter,
    const SimTK::State& state,
    const SimTK::DecorativeGeometry& geom,
    float fixupScaleFactor,
    const std::function<void(osc::SceneDecoration&&)>& out)
{
    GeometryImpl impl{meshCache, matter, state, fixupScaleFactor, out};
    geom.implementGeometry(impl);
}
