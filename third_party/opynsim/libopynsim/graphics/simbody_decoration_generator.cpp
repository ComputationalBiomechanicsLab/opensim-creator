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
#include <liboscar/maths/vector.h>
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
    inline constexpr float c_line_thickness = 0.005f;
    inline constexpr float c_frame_axis_length_rescale = 0.25f;
    inline constexpr float c_frame_axis_thickness = 0.0025f;

    // extracts scale factors from geometry
    osc::Vector3 get_scale_factors(const SimTK::DecorativeGeometry& geom)
    {
        // Use patched-in defaulting check for the edge-case where OpenSim
        // emits geometry with `-1.0` scale factors, which are used to mean
        // "default it", rather than to mean "flip it"
        // (ComputationalBiomechanicsLab/opensim-creator#1179).
        if (geom.hasDefaultedScaleFactors()) {
            return osc::Vector3{1.0f};
        }

        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i) {
            // filter out NaNs, but keep negative values, because some
            // users use negative scales to mimic mirror imaging
            // (ComputationalBiomechanicsLab/opensim-creator#974).
            sf[i] = not std::isnan(sf[i]) ? sf[i] : 0.0;
        }

        return osc::to<osc::Vector3>(sf);
    }

    float get_opacity(const SimTK::DecorativeGeometry& geometry)
    {
        const auto rv = static_cast<float>(geometry.getOpacity());
        return rv >= 0.0f ? rv : 1.0f;
    }

    // returns the color of `geometry`, with any defaults saturated to `1.0f`
    osc::Color get_color(const SimTK::DecorativeGeometry& geometry)
    {
        auto rgb = osc::to<osc::Vector3>(geometry.getColor());

        // Simbody uses `-1` to mean "use default`. We use a default of `1.0f`
        // whenever this, or a NaN, occurs.
        for (auto& component : rgb) {
            component = component >= 0.0f ? component : 1.0f;
        }
        return osc::Color{rgb, get_opacity(geometry)};
    }

    // Returns `true` if `geometry` has a defaulted color
    bool is_default_color(const SimTK::DecorativeGeometry& geometry)
    {
        return geometry.getColor() == SimTK::Vec3{-1.0, -1.0, -1.0};
    }

    osc::SceneDecorationFlags get_flags(const SimTK::DecorativeGeometry& geom)
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
    osc::Transform to_osc_transform_without_scaling(
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
        const int num_verts = mesh.getNumVertices();
        hash = osc::hash_combine(hash, osc::hash_of(num_verts));
        for (int vert = 0; vert < num_verts; ++vert)
        {
            hash = osc::hash_combine(hash, hash_of(mesh.getVertexPosition(vert)));
        }

        // combine face indices into mesh
        const int num_faces = mesh.getNumFaces();
        hash = osc::hash_combine(hash, osc::hash_of(num_faces));
        for (int face = 0; face < num_faces; ++face)
        {
            const int num_verts_in_face = mesh.getNumVerticesForFace(face);
            for (int face_vert = 0; face_vert < num_verts_in_face; ++face_vert)
            {
                hash = osc::hash_combine(hash, osc::hash_of(mesh.getFaceVertex(face, face_vert)));
            }
        }

        return hash;
    }

    // an implementation of SimTK::DecorativeGeometryImplementation that emits generic
    // triangle-mesh-based SystemDecorations that can be consumed by the rest of the UI
    class GeometryImpl final : public SimTK::DecorativeGeometryImplementation {
    public:
        GeometryImpl(
            osc::SceneCache& mesh_cache,
            const SimTK::SimbodyMatterSubsystem& matter,
            const SimTK::State& st,
            float fixup_scale_factor,
            const std::function<void(osc::SceneDecoration&&)>& out) :

            mesh_cache_{mesh_cache},
            matter_{matter},
            state_{st},
            fixup_scale_factor_{fixup_scale_factor},
            consumer_{out}
        {}

    private:
        osc::Transform to_osc_transform_without_scaling(const SimTK::DecorativeGeometry& d) const
        {
            return ::to_osc_transform_without_scaling(matter_, state_, d);
        }

        osc::Transform to_osc_transform(const SimTK::DecorativeGeometry& d) const
        {
            return to_osc_transform_without_scaling(d).with_scale(get_scale_factors(d));
        }

        void implementPointGeometry(const SimTK::DecorativePoint&) final
        {
            [[maybe_unused]] static const bool s_shown_warning_once = []()
            {
                osc::log_warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementLineGeometry(const SimTK::DecorativeLine& d) final
        {
            const osc::Transform t = to_osc_transform(d);
            const osc::Vector3 p1 = t * osc::to<osc::Vector3>(d.getPoint1());
            const osc::Vector3 p2 = t * osc::to<osc::Vector3>(d.getPoint2());

            const float thickness = c_line_thickness * fixup_scale_factor_;

            osc::Transform cylinder_xform = osc::cylinder_to_line_segment_transform({p1, p2}, thickness);
            cylinder_xform.scale *= t.scale;

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.cylinder_mesh(),
                .transform = cylinder_xform,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementBrickGeometry(const SimTK::DecorativeBrick& d) final
        {
            osc::Transform t = to_osc_transform(d);
            t.scale *= osc::to<osc::Vector3>(d.getHalfLengths());

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.brick_mesh(),
                .transform = t,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementCylinderGeometry(const SimTK::DecorativeCylinder& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());
            const auto half_height = static_cast<float>(d.getHalfHeight());

            osc::Transform t = to_osc_transform(d);
            t.scale *= osc::Vector3{radius, half_height , radius};

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.cylinder_mesh(),
                .transform = t,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementCircleGeometry(const SimTK::DecorativeCircle& d) final
        {
            const auto radius = static_cast<float>(d.getRadius());

            osc::Transform t = to_osc_transform(d);
            t.scale *= osc::Vector3{radius, radius, 1.0f};

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.circle_mesh(),
                .transform = t,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementSphereGeometry(const SimTK::DecorativeSphere& d) final
        {
            osc::Transform t = to_osc_transform(d);
            t.scale *= fixup_scale_factor_ * static_cast<float>(d.getRadius());

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.sphere_mesh(),
                .transform = t,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementEllipsoidGeometry(const SimTK::DecorativeEllipsoid& d) final
        {
            osc::Transform t = to_osc_transform(d);
            t.scale *= osc::to<osc::Vector3>(d.getRadii());

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.sphere_mesh(),
                .transform = t,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementFrameGeometry(const SimTK::DecorativeFrame& d) final
        {
            const osc::Transform t = to_osc_transform(d);

            // if the calling code explicitly sets the color of a frame as non-white, then
            // that override should be obeyed, rather than using OSC's custom coloring
            // scheme (#985).
            const std::optional<osc::Color> color_override = is_default_color(d)  or d.getColor() == SimTK::Vec3{1.0, 1.0, 1.0} ?
                std::optional<osc::Color>{} :
                get_color(d);

            // emit origin sphere
            {
                const float radius = 0.05f * c_frame_axis_length_rescale * fixup_scale_factor_;
                const osc::Transform sphere_xform = t.with_scale(radius);

                consumer_(osc::SceneDecoration{
                    .mesh = mesh_cache_.sphere_mesh(),
                    .transform = sphere_xform,
                    .shading = color_override ? *color_override : osc::Color::white(),
                    .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
                });
            }

            // emit leg cylinders
            const osc::Vector3 axis_lengths = t.scale * static_cast<float>(d.getAxisLength());
            const float leg_len = c_frame_axis_length_rescale * fixup_scale_factor_;
            const float leg_thickness = c_frame_axis_thickness * fixup_scale_factor_;
            const auto flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull;
            for (int axis = 0; axis < 3; ++axis) {
                osc::Vector3 direction = osc::Vector3{}.with_element(axis, 1.0f);

                const osc::LineSegment line_segment = {
                    t.translation,
                    t.translation + (leg_len * axis_lengths[axis] * normalize(transform_vector(t, direction)))
                };
                const osc::Transform leg_xform = cylinder_to_line_segment_transform(line_segment, leg_thickness);

                osc::Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[axis] = 1.0f;

                consumer_(osc::SceneDecoration{
                    .mesh = mesh_cache_.cylinder_mesh(),
                    .transform = leg_xform,
                    .shading = color_override ? *color_override : color,
                    .flags = flags,
                });
            }
        }

        void implementTextGeometry(const SimTK::DecorativeText&) final
        {
            [[maybe_unused]] static const bool s_shown_warning_once = []()
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
            const auto mesh_loader_func = [&d]() { return to_osc_mesh(d.getMesh()); };

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.get_mesh(id, mesh_loader_func),
                .transform = to_osc_transform(d),
                .shading = get_color(d),
                .flags = get_flags(d),  // no `SceneDecorationFlag::CanBackfaceCull`, because mesh data might be invalid (#318, #168)
            });
        }

        void implementMeshFileGeometry(const SimTK::DecorativeMeshFile& d) final
        {
            const std::string& path = d.getMeshFile();
            const auto mesh_loader = [&d](){ return to_osc_mesh(d.getMesh()); };

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.get_mesh(path, mesh_loader),
                .transform = to_osc_transform(d),
                .shading = get_color(d),
                .flags = get_flags(d),  // no `SceneDecorationFlag::CanBackfaceCull`, because mesh data might be invalid (#318, #168)
            });
        }

        void implementArrowGeometry(const SimTK::DecorativeArrow& d) final
        {
            const osc::Transform t = to_osc_transform_without_scaling(d);
            const osc::ArrowProperties p = {
                .start = t * osc::to<osc::Vector3>(d.getStartPoint()),
                .end = t * osc::to<osc::Vector3>(d.getEndPoint()),
                .tip_length = static_cast<float>(d.getTipLength()),
                .neck_thickness = fixup_scale_factor_ * static_cast<float>(d.getLineThickness()),
                .head_thickness = 1.75f * fixup_scale_factor_ * static_cast<float>(d.getLineThickness()),
                .color = get_color(d),
                .decoration_flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            };
            draw_arrow(mesh_cache_, p, consumer_);
        }

        void implementTorusGeometry(const SimTK::DecorativeTorus& d) final
        {
            const auto tube_center_radius = static_cast<float>(d.getTorusRadius());
            const auto tube_radius = static_cast<float>(d.getTubeRadius());

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.torus_mesh(tube_center_radius, tube_radius),
                .transform = to_osc_transform(d),
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        void implementConeGeometry(const SimTK::DecorativeCone& d) final
        {
            const osc::Transform t = to_osc_transform(d);

            auto pos_base = osc::to<osc::Vector3>(d.getOrigin());
            auto pos_dir = osc::to<osc::Vector3>(d.getDirection());

            const osc::Vector3 pos = transform_point(t, pos_base);
            const osc::Vector3 direction = normalize(transform_vector(t, pos_dir));

            const auto radius = static_cast<float>(d.getBaseRadius());
            const auto height = static_cast<float>(d.getHeight());

            osc::Transform cone_xform = osc::cylinder_to_line_segment_transform({pos, pos + height*direction}, radius);
            cone_xform.scale *= t.scale;

            consumer_(osc::SceneDecoration{
                .mesh = mesh_cache_.cone_mesh(),
                .transform = cone_xform,
                .shading = get_color(d),
                .flags = get_flags(d) | osc::SceneDecorationFlag::CanBackfaceCull,
            });
        }

        osc::SceneCache& mesh_cache_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        const SimTK::SimbodyMatterSubsystem& matter_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        const SimTK::State& state_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        float fixup_scale_factor_;
        const std::function<void(osc::SceneDecoration&&)>& consumer_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    };
}

void opyn::generate_decorations(
    osc::SceneCache& mesh_cache,
    const SimTK::SimbodyMatterSubsystem& matter,
    const SimTK::State& state,
    const SimTK::DecorativeGeometry& geom,
    float fixup_scale_factor,
    const std::function<void(osc::SceneDecoration&&)>& out)
{
    GeometryImpl impl{mesh_cache, matter, state, fixup_scale_factor, out};
    geom.implementGeometry(impl);
}
