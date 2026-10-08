#include "open_sim_decoration_generator.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/graphics/component_abs_path_decoration_tagger.h>
#include <libopynsim/graphics/component_scene_decoration_flags_tagger.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/simbody_decoration_generator.h>
#include <libopynsim/documents/custom_components/custom_decoration_generator.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/closed_interval.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/line_segment.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/quaternion_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/exception_helpers.h>
#include <liboscar/utilities/perf.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/ForceAdapter.h>
#include <OpenSim/Simulation/Model/ForceConsumer.h>
#include <OpenSim/Simulation/Model/ForceProducer.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Ligament.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/Model/PathSpring.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/Scholz2015GeometryPath.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

using osc::AABB;
using osc::ArrowProperties;
using osc::ClosedInterval;
using osc::Color;
using osc::CoordinateDirection;
using osc::Mesh;
using osc::Ray;
using osc::SceneCache;
using osc::SceneDecoration;
using osc::SceneDecorationFlag;
using osc::SceneDecorationFlags;
using osc::Transform;
using osc::Vector3;
using osc::abs;
using osc::cylinder_to_line_segment_transform;
using osc::dimensions_of;
using osc::equal_within_scaled_epsilon;
using osc::literals::operator ""_deg;
using osc::log_warn;
using osc::max;
using osc::min;
using osc::num_options;
using osc::pi_v;
using osc::saturate;
using osc::sqrt;
using osc::to;
using osc::unit_interval;

namespace
{
    // constants
    inline constexpr float c_geometry_path_base_radius = 0.005f;
    inline constexpr float c_force_arrow_length_scale = 0.0025f;
    inline constexpr float c_torque_arrow_length_scale = 0.01f;
    inline constexpr Color c_effective_line_of_action_color = Color::green();
    inline constexpr Color c_anatomical_line_of_action_color = Color::red();
    inline constexpr Color c_body_force_arrow_color = Color::yellow();
    inline constexpr Color c_body_torque_arrow_color = Color::orange();
    inline constexpr Color c_point_force_arrow_color = Color::muted_yellow();  // note: should be similar to body force arrows
    inline constexpr Color c_station_color = Color::red();
    inline constexpr Color c_scapulothoracic_joint_color =  Color::yellow().with_alpha(0.2f);
    inline constexpr Color c_center_of_mass_first_color = Color::lighter_grey();
    inline constexpr Color c_center_of_mass_second_color = Color::darker_grey();

    // helper: convert a physical frame's transform to ground into an Transform
    Transform transform_in_ground(const OpenSim::Frame& frame, const SimTK::State& state)
    {
        return to<Transform>(frame.getTransformInGround(state));
    }

    Color extract_color(const OpenSim::Appearance& appearance)
    {
        const SimTK::Vec3& rgb = appearance.get_color();
        return {
            static_cast<float>(rgb[0]),
            static_cast<float>(rgb[1]),
            static_cast<float>(rgb[2]),
            static_cast<float>(appearance.get_opacity()),
        };
    }

    Color get_geometry_path_color(const OpenSim::AbstractGeometryPath& gp, const SimTK::State& st)
    {
        // returns the same color that OpenSim emits (which is usually just activation-based,
        // but might change in future versions of OpenSim)
        return Color{to<Vector3>(gp.getColor(st)), static_cast<float>(gp.get_Appearance().get_opacity())};
    }

    // helper: calculates the radius of a muscle based on isometric force
    //
    // similar to how SCONE does it, so that users can compare between the two apps
    float get_scone_style_automatic_muscle_radius_calc(const OpenSim::Muscle& m)
    {
        const auto f = static_cast<float>(m.getMaxIsometricForce());
        const float specific_tension = 0.25e6f;  // magic number?
        const float pcsa = f / specific_tension;
        const float width_factor = 0.25f;
        return width_factor * sqrt(pcsa / pi_v<float>);
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float get_muscle_size(
        const OpenSim::Muscle& musc,
        float fixup_scale_factor,
        MuscleSizingStyle s)
    {
        switch (s) {
        case MuscleSizingStyle::PcsaDerived:
            return get_scone_style_automatic_muscle_radius_calc(musc) * fixup_scale_factor;
        case MuscleSizingStyle::Fixed:
        default:
            return c_geometry_path_base_radius * fixup_scale_factor;
        }
    }

    template<MuscleColorSource>
    float muscle_color_source_value_for(const OpenSim::Muscle&, const SimTK::State&);
    template<>
    float muscle_color_source_value_for<MuscleColorSource::Activation>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getActivation(state));
    }
    template<>
    float muscle_color_source_value_for<MuscleColorSource::AppearanceProperty>(const OpenSim::Muscle&, const SimTK::State&)
    {
        return 1.0f;
    }
    template<>
    float muscle_color_source_value_for<MuscleColorSource::Excitation>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getExcitation(state));
    }
    template<>
    float muscle_color_source_value_for<MuscleColorSource::Force>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getActuation(state) / muscle.getMaxIsometricForce());
    }
    template<>
    float muscle_color_source_value_for<MuscleColorSource::FiberLength>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        const auto nfl = static_cast<float>(muscle.getNormalizedFiberLength(state));  // 1.0f == ideal length
        float fl = nfl - 1.0f;
        fl = abs(fl);
        fl = min(fl, 1.0f);
        return fl;
    }

    // A lookup abstraction for figuring out the color factor of a muscle along a ramp.
    class MuscleColorFactorLookup final {
    public:
        MuscleColorFactorLookup(
            const OpenSim::Model& model,
            const SimTK::State& state,
            MuscleColorSource color_source,
            MuscleColorSourceScaling scaling) :

            getter_{choose_getter(color_source)},
            scaling_range_{choose_scaling_range(model, state, getter_, scaling)}
        {}

        // Returns a number in the range [0.0, 1.0] that describes the suggested position
        // a muscle's color should be on a color ramp (e.g. from blue to red).
        float lookup(const OpenSim::Muscle& muscle, const SimTK::State& state) const
        {
            const float v = getter_(muscle, state);
            const float t = scaling_range_.normalized_interpolant_at(v);
            return saturate(t);
        }

    private:
        using MuscleColorFactorGetter = float (*)(const OpenSim::Muscle&, const SimTK::State&);

        static MuscleColorFactorGetter choose_getter(MuscleColorSource source)
        {
            switch (source) {
            case MuscleColorSource::AppearanceProperty: return muscle_color_source_value_for<MuscleColorSource::AppearanceProperty>;
            case MuscleColorSource::Activation:         return muscle_color_source_value_for<MuscleColorSource::Activation>;
            case MuscleColorSource::Excitation:         return muscle_color_source_value_for<MuscleColorSource::Excitation>;
            case MuscleColorSource::Force:              return muscle_color_source_value_for<MuscleColorSource::Force>;
            case MuscleColorSource::FiberLength:        return muscle_color_source_value_for<MuscleColorSource::FiberLength>;
            default:                                    return muscle_color_source_value_for<MuscleColorSource::Default>;
            }
        }

        static ClosedInterval<float> choose_scaling_range(
            const OpenSim::Model& model,
            const SimTK::State& state,
            const MuscleColorFactorGetter& getter,
            MuscleColorSourceScaling scaling)
        {
            static_assert(num_options<MuscleColorSourceScaling>() == 2);

            switch (scaling) {
            case MuscleColorSourceScaling::None:      return unit_interval<float>();
            case MuscleColorSourceScaling::ModelWide: return calculate_model_wide_scaling_range(model, state, getter);
            default:                                  return unit_interval<float>();
            }
        }

        static ClosedInterval<float> calculate_model_wide_scaling_range(
            const OpenSim::Model& model,
            const SimTK::State& state,
            const MuscleColorFactorGetter& getter)
        {
            std::optional<ClosedInterval<float>> accumulator;
            for (const auto& muscle : model.getComponentList<OpenSim::Muscle>()) {
                accumulator = bounding_interval_of(accumulator, getter(muscle, state));
            }
            return accumulator.value_or(unit_interval<float>());
        }

        MuscleColorFactorGetter getter_;
        ClosedInterval<float> scaling_range_;
    };
}

// geometry handlers
namespace
{

    // a datastructure that is shared to all decoration-generation functions
    //
    // effectively, this is shared state/functions that each low-level decoration
    // generation routine can use to emit low-level primitives (e.g. spheres)
    class RendererState final {
    public:
        RendererState(
            SceneCache& mesh_cache,
            const OpenSim::Model& model,
            const SimTK::State& state,
            const OpenSimDecorationOptions& opts,
            float fixup_scale_factor,
            const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out) :

            mesh_cache_{mesh_cache},
            model_{model},
            state_{state},
            opts_{opts},
            fixup_scale_factor_{fixup_scale_factor},
            out_{out}
        {}

        SceneCache& upd_scene_cache()
        {
            return mesh_cache_;
        }

        const Mesh& sphere_mesh() const
        {
            return sphere_mesh_;
        }

        const Mesh& sphere_octant_mesh() const
        {
            return sphere_octant_mesh_;
        }

        const Mesh& uncapped_cylinder_mesh() const
        {
            return uncapped_cylinder_mesh_;
        }

        const OpenSim::ModelDisplayHints& get_model_display_hints() const
        {
            return model_display_hints_;
        }

        bool get_show_path_points() const
        {
            return show_path_points_;
        }

        const SimTK::SimbodyMatterSubsystem& get_matter_subsystem() const
        {
            return matter_subsystem_;
        }

        const SimTK::State& get_state() const
        {
            return state_;
        }

        const OpenSimDecorationOptions& get_options() const
        {
            return opts_;
        }

        const OpenSim::Model& get_model() const
        {
            return model_;
        }

        float get_fixup_scale_factor() const
        {
            return fixup_scale_factor_;
        }

        void consume(const OpenSim::Component& component, SceneDecoration&& dec)
        {
            // Filter out any scene decorations that have transforms that have any
            // NaN elements. This is a precaution to guard against bad maths in
            // OpenSim or OSC's custom decoration generator code (#976).
            if (any_element_is_nan(dec.transform)) {
                return;
            }
            out_(component, std::move(dec));
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emit_generic_decorations(
            const OpenSim::Component& component_to_render,
            const OpenSim::Component& component_to_link_to,
            float fixup_scale_factor)
        {
            const std::function<void(SceneDecoration&&)> callback = [this, &component_to_link_to](SceneDecoration&& dec)
            {
                consume(component_to_link_to, std::move(dec));
            };

            geom_list_.clear();
            component_to_render.generateDecorations(
                true,
                get_model_display_hints(),
                get_state(),
                geom_list_
            );
            for (const SimTK::DecorativeGeometry& geom : geom_list_)
            {
                generate_decorations(
                    upd_scene_cache(),
                    get_matter_subsystem(),
                    get_state(),
                    geom,
                    fixup_scale_factor,
                    callback
                );
            }

            geom_list_.clear();
            component_to_render.generateDecorations(
                false,
                get_model_display_hints(),
                get_state(),
                geom_list_
            );
            for (const SimTK::DecorativeGeometry& geom : geom_list_)
            {
                generate_decorations(
                    upd_scene_cache(),
                    get_matter_subsystem(),
                    get_state(),
                    geom,
                    fixup_scale_factor,
                    callback
                );
            }
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emit_generic_decorations(
            const OpenSim::Component& component_to_render,
            const OpenSim::Component& component_to_link_to)
        {
            emit_generic_decorations(component_to_render, component_to_link_to, get_fixup_scale_factor());
        }

        Color calc_muscle_color(const OpenSim::Muscle& muscle)
        {
            if (get_options().get_muscle_color_source() == MuscleColorSource::AppearanceProperty) {
                // early-out: the muscle has a constant, Appearance-defined color
                return extract_color(muscle.getPath().get_Appearance());
            }

            const float t = muscle_color_source_scaling_lookup_.lookup(muscle, get_state());

            // Note: always take the path `Appearance` opacity into account, even if the color
            // is being computed from the state (semi-related: #1166).
            const auto alpha = static_cast<float>(muscle.getPath().get_Appearance().get_opacity());
            const Color zero_color = {50.0f / 255.0f, 50.0f / 255.0f, 166.0f / 255.0f, alpha};
            const Color full_color = {255.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, alpha};
            return lerp(zero_color, full_color, t);
        }

        SceneDecorationFlags calc_geometry_path_flags(const OpenSim::AbstractGeometryPath& gp)
        {
            // Note: this assumes `OpenSim::Appearance::is_visible` is handled at a higher
            // level (it should remove the decoration from the scene graph entirely).

            SceneDecorationFlags rv = {SceneDecorationFlag::Default, SceneDecorationFlag::CanBackfaceCull};

            switch (gp.get_Appearance().get_representation()) {
                case OpenSim::VisualRepresentation::DrawWireframe: { rv |= SceneDecorationFlag::OnlyWireframe; break; }
                case OpenSim::VisualRepresentation::Hide:          { rv |= SceneDecorationFlag::Hidden;        break; }
                default:                                           { break; }
            }
            return rv;
        }

        SceneDecorationFlags calc_muscle_flags(const OpenSim::Muscle& muscle)
        {
            return calc_geometry_path_flags(muscle.getPath());
        }
    private:
        SceneCache& mesh_cache_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        Mesh sphere_mesh_ = mesh_cache_.sphere_mesh();
        Mesh sphere_octant_mesh_ = mesh_cache_.sphere_octant_mesh();
        Mesh uncapped_cylinder_mesh_ = mesh_cache_.uncapped_cylinder_mesh();
        const OpenSim::Model& model_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        const OpenSim::ModelDisplayHints& model_display_hints_ = model_.getDisplayHints();  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        bool show_path_points_ = model_display_hints_.get_show_path_points();
        const SimTK::SimbodyMatterSubsystem& matter_subsystem_ = model_.getSystem().getMatterSubsystem();  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        const SimTK::State& state_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        const OpenSimDecorationOptions& opts_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        float fixup_scale_factor_;
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        SimTK::Array_<SimTK::DecorativeGeometry> geom_list_;
        MuscleColorFactorLookup muscle_color_source_scaling_lookup_{model_, state_, opts_.get_muscle_color_source(), opts_.get_muscle_color_source_scaling()};
    };

    // An `OpenSim::ForceConsumer` that emits `SceneDecoration` arrows that represent
    // each force vector it has consumed.
    //
    // Callers should also call `emit_accumulated_body_spatial_vecs` after `produceForces`
    // has completed, because this implementation automatically merges body forces on
    // the same body together.
    class SceneDecorationGeneratingForceConsumer : public OpenSim::ForceConsumer {
    public:
        explicit SceneDecorationGeneratingForceConsumer(
            RendererState* renderer_state,
            const OpenSim::ForceProducer* force_producer) :
            renderer_state_{renderer_state},
            associated_force_producer_{force_producer}
        {}

        // Emit any body forces that were accumulated during the production phase
        void emit_accumulated_body_spatial_vecs(const SimTK::State& state)
        {
            for (const auto& [bodyPtr, spatialVec] : accumulated_body_spatial_vecs_) {
                handle_body_torque(state, *bodyPtr, spatialVec[0]);
                handle_body_force(state, *bodyPtr, spatialVec[1]);
            }
        }
    private:
        // Implements `ForceConsumer` API by generating equivalent `SceneDecoration`s for
        // the body torque + force (separately).
        void implConsumeBodySpatialVec(
            const SimTK::State&,
            const OpenSim::PhysicalFrame& body,
            const SimTK::SpatialVec& spatial_vec) override
        {
            if (accumulated_body_spatial_vecs_.empty()) {
                // Lazily reserve memory for the accumulated body forces lookup. Most
                // `ForceProducer`s will only touch a few `Body`s, 8 is a guess on the
                // most likely upper limit.
                accumulated_body_spatial_vecs_.reserve(8);
            }

            // Accumulate the body forces, rather than emitting them seperately, because
            // it makes the visualization less cluttered.
            auto& accumulator = accumulated_body_spatial_vecs_.try_emplace(&body, SimTK::SpatialVec{SimTK::Vec3{0.0}, SimTK::Vec3{0.0}}).first->second;
            accumulator += spatial_vec;
        }

        // Implements `ForceConsumer` API by generating equivalent `SceneDecoration`s for
        // the point force (incl. conversion to a resultant body force)
        void implConsumePointForce(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& frame,
            const SimTK::Vec3& point,
            const SimTK::Vec3& force_in_ground) override
        {
            if (equal_within_scaled_epsilon(force_in_ground.normSqr(), 0.0)) {
                return;  // zero/small force provided: skip it
            }

            // if requested, generate an arrow decoration for the point force
            if (renderer_state_->get_options().get_should_show_point_forces()) {
                const float fixup_scale_factor = renderer_state_->get_fixup_scale_factor();
                const SimTK::Vec3 position_in_ground = frame.findStationLocationInGround(state, point);
                const ArrowProperties arrow_properties = {
                    .start = to<Vector3>(position_in_ground),
                    .end = to<Vector3>(position_in_ground + (fixup_scale_factor * c_force_arrow_length_scale * force_in_ground)),
                    .tip_length = 0.015f * fixup_scale_factor,
                    .neck_thickness = 0.006f * fixup_scale_factor,
                    .head_thickness = 0.01f * fixup_scale_factor,
                    .color = c_point_force_arrow_color,
                    .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
                };

                draw_arrow(renderer_state_->upd_scene_cache(), arrow_properties, [this](SceneDecoration&& decoration)
                {
                    renderer_state_->consume(*associated_force_producer_, std::move(decoration));
                });
            }

            // accumulate associated body force
            {
                // maths ripped from `SimbodyMatterSubsystem::addInStationForce`
                //
                // https://github.com/simbody/simbody/blob/34b0ac47e6252457733a503c234b2daf1c596d81/Simbody/src/SimbodyMatterSubsystem.cpp#L2190

                const auto& base_frame = dynamic_cast<const OpenSim::PhysicalFrame&>(frame.findBaseFrame());
                const SimTK::Rotation& r_gb = base_frame.getTransformInGround(state).R();
                const SimTK::Vec3 torque = (r_gb * point) % force_in_ground;
                implConsumeBodySpatialVec(state, base_frame, SimTK::SpatialVec{torque, force_in_ground});
            }
        }

        // Helper method for drawing the torque part of a `SimTK::SpatialVec`
        void handle_body_torque(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& body,
            const SimTK::Vec3& torque_in_ground)
        {
            if (not renderer_state_->get_options().get_should_show_force_angular_component()) {
                return;  // the caller has opted out of showing torques on bodies
            }
            if (equal_within_scaled_epsilon(torque_in_ground.normSqr(), 0.0)) {
                return;  // zero/small torque provided: skip it
            }

            const float fixup_scale_factor = renderer_state_->get_fixup_scale_factor();
            const SimTK::Transform& frame2ground = body.getTransformInGround(state);
            const ArrowProperties arrow_properties = {
                .start = to<Vector3>(frame2ground * SimTK::Vec3{0.0}),
                .end = to<Vector3>(frame2ground * (fixup_scale_factor * c_torque_arrow_length_scale * torque_in_ground)),
                .tip_length = (fixup_scale_factor*0.015f),
                .neck_thickness = (fixup_scale_factor*0.006f),
                .head_thickness = (fixup_scale_factor*0.01f),
                .color = c_body_torque_arrow_color,
                .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
            };
            draw_arrow(renderer_state_->upd_scene_cache(), arrow_properties, [this](SceneDecoration&& decoration)
            {
                renderer_state_->consume(*associated_force_producer_, std::move(decoration));
            });
        }

        // Helper method for drawing the force part of a `SimTK::SpatialVec`
        void handle_body_force(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& body,
            const SimTK::Vec3& force_in_ground)
        {
            if (not renderer_state_->get_options().get_should_show_force_linear_component()) {
                return;  // the caller has opted out of showing forces on bodies
            }
            if (equal_within_scaled_epsilon(force_in_ground.normSqr(), 0.0)) {
                return;  // zero/small force provided: skip it
            }

            const float fixup_scale_factor = renderer_state_->get_fixup_scale_factor();
            const SimTK::Transform& frame2ground = body.getTransformInGround(state);
            const ArrowProperties arrow_properties = {
                .start = to<Vector3>(frame2ground.p()),
                .end =  to<Vector3>(frame2ground.p() + (fixup_scale_factor * c_force_arrow_length_scale * force_in_ground)),
                .tip_length = (fixup_scale_factor*0.015f),
                .neck_thickness = (fixup_scale_factor*0.006f),
                .head_thickness = (fixup_scale_factor*0.01f),
                .color = c_body_force_arrow_color,
                .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
            };
            draw_arrow(renderer_state_->upd_scene_cache(), arrow_properties, [this](SceneDecoration&& decoration)
            {
                renderer_state_->consume(*associated_force_producer_, std::move(decoration));
            });
        }

        RendererState* renderer_state_;
        OpenSim::ForceProducer const* associated_force_producer_;
        std::unordered_map<const OpenSim::PhysicalFrame*, SimTK::SpatialVec> accumulated_body_spatial_vecs_;
    };

    // OSC-specific decoration handler that decorates the body forces/torques applied by
    // an `OpenSim::Force` using the `OpenSim::Force::computeForce` API
    //
    // Note: if an `OpenSim::Force` is actually an `OpenSim::ForceProducer`, then use that
    //       API instead - this code is here to support "legacy" forces that haven't
    //       implemented that API yet. An overview of the `ForceProducer` API explains the
    //       relevant motivations etc: https://github.com/opensim-org/opensim-core/pull/3891
    void generate_body_spatial_vector_arrow_decorations_for_forces_that_only_have_compute_force_method(
        RendererState& rs,
        const OpenSim::Force& force)
    {
        const bool show_forces = rs.get_options().get_should_show_force_linear_component();
        const bool show_torques = rs.get_options().get_should_show_force_angular_component();
        if (not show_forces and not show_torques) {
            return;  // caller doesn't want to draw this
        }

        if (not force.appliesForce(rs.get_state())) {
            return;  // the `Force` does not apply a force
        }

        // this is a very heavy-handed way of getting the relevant information, because
        // OpenSim's `Force` implementation implicitly assumes that all body forces are
        // available in one contiguous vector

        const SimTK::SimbodyMatterSubsystem& matter = rs.get_matter_subsystem();
        const SimTK::State& state = rs.get_state();

        const OpenSim::ForceAdapter adapter{force};
        SimTK::Vector_<SimTK::SpatialVec> body_forces(matter.getNumBodies(), SimTK::SpatialVec{SimTK::Vec3{0.0}, SimTK::Vec3{0.0}});
        SimTK::Vector_<SimTK::Vec3> particle_forces(matter.getNumParticles(), SimTK::Vec3{0.0});  // (unused)
        SimTK::Vector mobility_forces(matter.getNumMobilities(), double{});  // (unused)

        adapter.calcForce(
            state,
            body_forces,
            particle_forces,  // unused, but required
            mobility_forces   // unused, but required
        );

        const float fixup_scale_factor = rs.get_fixup_scale_factor();
        for (SimTK::MobilizedBodyIndex body_idx{0}; body_idx < body_forces.size(); ++body_idx) {

            const SimTK::MobilizedBody& mobod = matter.getMobilizedBody(body_idx);
            const SimTK::Transform mobod2ground = mobod.getBodyTransform(state);

            // if applicable, handle drawing the linear component of force as an arrow
            if (show_forces) {
                const SimTK::Vec3 force_vec = body_forces[body_idx][1];
                if (equal_within_scaled_epsilon(force_vec.normSqr(), 0.0)) {
                    continue;  // no translational force applied
                }

                const ArrowProperties arrow_properties = {
                    .start = to<Vector3>(mobod2ground.p()),
                    .end = to<Vector3>(mobod2ground.p() + (fixup_scale_factor * c_force_arrow_length_scale * force_vec)),
                    .tip_length = (fixup_scale_factor*0.015f),
                    .neck_thickness = (fixup_scale_factor*0.006f),
                    .head_thickness = (fixup_scale_factor*0.01f),
                    .color = c_body_force_arrow_color,
                    .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
                };
                draw_arrow(rs.upd_scene_cache(), arrow_properties, [&force, &rs](SceneDecoration&& decoration)
                {
                    rs.consume(force, std::move(decoration));
                });
            }

            // if applicable, handle drawing the angular component of force as an arrow
            if (show_torques) {
                const SimTK::Vec3 torque_vec = body_forces[body_idx][0];
                if (equal_within_scaled_epsilon(torque_vec.normSqr(), 0.0)) {
                    continue;  // no translational force applied
                }

                const ArrowProperties arrow_properties = {
                    .start = to<Vector3>(mobod2ground * SimTK::Vec3{0.0}),
                    .end = to<Vector3>(mobod2ground * (fixup_scale_factor * c_torque_arrow_length_scale * torque_vec)),
                    .tip_length = (fixup_scale_factor*0.015f),
                    .neck_thickness = (fixup_scale_factor*0.006f),
                    .head_thickness = (fixup_scale_factor*0.01f),
                    .color = c_body_torque_arrow_color,
                    .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
                };
                draw_arrow(rs.upd_scene_cache(), arrow_properties, [&force, &rs](SceneDecoration&& decoration)
                {
                    rs.consume(force, std::move(decoration));
                });
            }
        }
    }

    // Geneerates arrow decorations that represent the provided `OpenSim::ForceProducer`'s
    // effect on the model (depending on caller-provided options, etc.)
    //
    // - #907 is related to this. Previously, this codebase had special code for pulling
    //   point-force vectors out of `OpenSim::GeometryPath`s, but this was later unified
    //   for all forces when the `ForceProducer` API was merged: https://github.com/opensim-org/opensim-core/pull/3891
    void generate_force_arrow_decorations_from_force_producer(
        RendererState& rs,
        const OpenSim::ForceProducer& force_producer)
    {
        if (not force_producer.appliesForce(rs.get_state())) {
            return;  // the `ForceProducer` is currently disabled
        }

        if (not rs.get_options().get_should_show_point_forces() and
            not rs.get_options().get_should_show_force_linear_component() and
            not rs.get_options().get_should_show_force_angular_component()) {

            return;  // caller doesn't want to draw any kind of force vector
        }

        SceneDecorationGeneratingForceConsumer consumer{&rs, &force_producer};
        force_producer.produceForces(rs.get_state(), consumer);
        consumer.emit_accumulated_body_spatial_vecs(rs.get_state());
    }

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void handle_point_to_point_spring(
        RendererState& rs,
        const OpenSim::PointToPointSpring& p2p)
    {
        if (not rs.get_options().get_should_show_point_to_point_springs()) {
            return;
        }

        const Vector3 p1 = transform_in_ground(p2p.getBody1(), rs.get_state()) * to<Vector3>(p2p.getPoint1());
        const Vector3 p2 = transform_in_ground(p2p.getBody2(), rs.get_state()) * to<Vector3>(p2p.getPoint2());

        const float radius = c_geometry_path_base_radius * rs.get_fixup_scale_factor();

        rs.consume(p2p, SceneDecoration{
            .mesh = rs.upd_scene_cache().cylinder_mesh(),
            .transform = cylinder_to_line_segment_transform({p1, p2}, radius),
            .shading = Color::light_grey(),
            .flags = {SceneDecorationFlag::Default, SceneDecorationFlag::CanBackfaceCull},
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void handle_station(
        RendererState& rs,
        const OpenSim::Station& s)
    {
        const float radius = rs.get_fixup_scale_factor() * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        rs.consume(s, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = {
                .scale = Vector3{radius},
                .translation = to<Vector3>(s.getLocationInGround(rs.get_state())),
            },
            .shading = c_station_color,
            .flags = {SceneDecorationFlag::Default, SceneDecorationFlag::CanBackfaceCull},
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void handle_scapulothoracic_joint(
        RendererState& rs,
        const OpenSim::ScapulothoracicJoint& scapulo_joint)
    {
        Transform t = transform_in_ground(scapulo_joint.getParentFrame(), rs.get_state());
        t.scale = to<Vector3>(scapulo_joint.get_thoracic_ellipsoid_radii_x_y_z());

        rs.consume(scapulo_joint, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = t,
            .shading = c_scapulothoracic_joint_color,
            .flags = {SceneDecorationFlag::Default, SceneDecorationFlag::CanBackfaceCull},
        });
    }

    // OSC-specific decoration handler for body centers of mass
    void handle_body_centers_of_mass(
        RendererState& rs,
        const OpenSim::Body& b)
    {
        if (not rs.get_options().get_should_show_centers_of_mass()) {
            return;
        }
        if (b.getMassCenter() == SimTK::Vec3{0.0}) {
            return;
        }

        // draw a COM by drawing 8 sphere octants to form a sphere
        // with two alternating colors (standard visual notation used
        // by engineers etc.)

        const float radius = rs.get_fixup_scale_factor() * 0.0075f;
        Transform t = transform_in_ground(b, rs.get_state());
        t.translation = t * to<Vector3>(b.getMassCenter());
        t.scale = Vector3{radius};
        constexpr SceneDecorationFlags flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull};

        // Draw four sphere octants in the first color
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t,
            .shading = c_center_of_mass_first_color,
            .flags = flags,
        });
        for (auto&& axis : {CoordinateDirection::x(), CoordinateDirection::y(), CoordinateDirection::z()}) {
            rs.consume(b, SceneDecoration{
                .mesh = rs.sphere_octant_mesh(),
                .transform = t.with_rotation(t.rotation * angle_axis(180_deg, axis)),
                .shading = c_center_of_mass_first_color,
                .flags = flags,
            });
        }

        // Draw four sphere octants with the second color
        t = t.with_rotation(t.rotation * angle_axis(90_deg, CoordinateDirection::x()));  // start rotated 90 degrees
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t,
            .shading = c_center_of_mass_second_color,
            .flags = flags,
        });
        for (auto&& axis : {CoordinateDirection::x(), CoordinateDirection::y(), CoordinateDirection::z()}) {
            rs.consume(b, SceneDecoration{
                .mesh = rs.sphere_octant_mesh(),
                .transform = t.with_rotation(t.rotation * angle_axis(180_deg, axis)),
                .shading = c_center_of_mass_second_color,
                .flags = flags,
            });
        }
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void handle_body(
        RendererState& rs,
        const OpenSim::Body& b)
    {
        handle_body_centers_of_mass(rs, b);  // CoMs are handled by OSC
        rs.emit_generic_decorations(b, b);  // bodies are emitted by OpenSim
    }

    // OSC-specific decoration handler for Muscle+Fiber representation of an `OpenSim::Muscle`
    void handle_muscle_fibers_and_tendons(
        RendererState& rs,
        const OpenSim::Muscle& muscle)
    {
        const std::vector<OpenSim::AbstractGeometryPath::DecorativePathPoint> pps =
            muscle.getPath().getDecorativePathPoints(rs.get_state());
        if (pps.empty()) {
            return;  // edge-case: there are no points in the muscle path
        }

        // precompute various coefficients, reused meshes, helpers, etc.

        const float fixup_scale_factor = rs.get_fixup_scale_factor();

        const float fiber_ui_radius = get_muscle_size(
            muscle,
            fixup_scale_factor,
            rs.get_options().get_muscle_sizing_style()
        );
        const float tendon_ui_radius = 0.618f * fiber_ui_radius;  // or fixup_scale_factor * 0.005f;

        const Color fiber_color = rs.calc_muscle_color(muscle);
        const Color tendon_color = {
            204.0f/255.0f,
            203.0f/255.0f,
            200.0f/255.0f,
            static_cast<float>(muscle.getPath().get_Appearance().get_opacity()),  // Always take user-enacted opacity into account (#1166).
        };
        const SceneDecorationFlags flags = rs.calc_muscle_flags(muscle);

        const SceneDecoration tendon_sphere_prototype = {
            .mesh = rs.sphere_mesh(),
            .transform = {.scale = Vector3{tendon_ui_radius}},
            .shading = tendon_color,
            .flags = flags,
        };
        const SceneDecoration tendon_cylinder_prototype = {
            .mesh = rs.uncapped_cylinder_mesh(),
            .shading = tendon_color,
            .flags = flags,
        };
        const SceneDecoration fiber_sphere_prototype = {
            .mesh = rs.sphere_mesh(),
            .transform = {.scale = Vector3{fiber_ui_radius}},
            .shading = fiber_color,
            .flags = flags,
        };
        const SceneDecoration fiber_cylinder_prototype = {
            .mesh = rs.uncapped_cylinder_mesh(),
            .shading = fiber_color,
            .flags = flags,
        };

        const auto emit_tendon_sphere = [&](const OpenSim::AbstractGeometryPath::DecorativePathPoint& p)
        {
            const OpenSim::Component* c = &muscle;
            if (p.getAssociatedComponent()) {
                c = p.getAssociatedComponent();
            }
            rs.consume(*c, tendon_sphere_prototype.with_translation(to<Vector3>(p.getLocationInGround())));
        };
        const auto emit_tendon_cylinder = [&](const SimTK::Vec3& p1, const SimTK::Vec3& p2)
        {
            const Transform xform = cylinder_to_line_segment_transform({to<Vector3>(p1), to<Vector3>(p2)}, tendon_ui_radius);
            rs.consume(muscle, tendon_cylinder_prototype.with_transform(xform));
        };
        auto emit_fiber_sphere = [&](const OpenSim::AbstractGeometryPath::DecorativePathPoint& p)
        {
            const OpenSim::Component* c = &muscle;
            if (p.getAssociatedComponent()) {
                c = p.getAssociatedComponent();
            }
            rs.consume(*c, fiber_sphere_prototype.with_translation(to<Vector3>(p.getLocationInGround())));
        };
        auto emit_fiber_cylinder = [&](const SimTK::Vec3& p1, const SimTK::Vec3& p2)
        {
            const Transform xform = cylinder_to_line_segment_transform({to<Vector3>(p1), to<Vector3>(p2)}, fiber_ui_radius);
            rs.consume(muscle, fiber_cylinder_prototype.with_transform(xform));
        };

        // start emitting the path

        if (pps.size() == 1) {
            // edge-case: this shouldn't happen but, just to be safe...
            emit_fiber_sphere(pps.front());
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it and split it into tendon-fiber-tendon
        const float tendon_len = max(0.0f, static_cast<float>(muscle.getTendonLength(rs.get_state()) * 0.5));
        const float fiber_len = max(0.0f, static_cast<float>(muscle.getFiberLength(rs.get_state())));
        const float fiber_end = tendon_len + fiber_len;
        const bool has_tendon_spheres = tendon_len > 0.0f;

        size_t i = 1;
        OpenSim::AbstractGeometryPath::DecorativePathPoint prev_point = pps.front();
        float prev_traversal_position = 0.0f;

        // emit first sphere for first tendon
        if (prev_traversal_position < tendon_len) {
            emit_tendon_sphere(prev_point);  // emit first tendon sphere
        }

        // emit remaining cylinders + spheres for first tendon
        while (i < pps.size() && prev_traversal_position < tendon_len) {

            const OpenSim::AbstractGeometryPath::DecorativePathPoint& point = pps[i];
            const SimTK::Vec3 prev_to_pos = point.getLocationInGround() - prev_point.getLocationInGround();
            const auto prev_to_pos_len = static_cast<float>(prev_to_pos.norm());
            const float traversal_pos = prev_traversal_position + prev_to_pos_len;
            const float excess = traversal_pos - tendon_len;

            if (excess > 0.0f) {
                const float scaler = (prev_to_pos_len - excess)/prev_to_pos_len;
                const SimTK::Vec3 tendon_end = prev_point.getLocationInGround() + scaler * prev_to_pos;

                emit_tendon_cylinder(prev_point.getLocationInGround(), tendon_end);
                emit_tendon_sphere(OpenSim::AbstractGeometryPath::DecorativePathPoint{tendon_end});

                prev_point.setLocationInGround(tendon_end);
                prev_traversal_position = tendon_len;
            }
            else {
                emit_tendon_cylinder(prev_point.getLocationInGround(), point.getLocationInGround());
                emit_tendon_sphere(point);

                i++;
                prev_point = point;
                prev_traversal_position = traversal_pos;
            }
        }

        // emit first sphere for fiber
        if (i < pps.size() && prev_traversal_position < fiber_end) {
            // label the sphere if no tendon spheres were previously emitted
            emit_fiber_sphere(has_tendon_spheres ? OpenSim::AbstractGeometryPath::DecorativePathPoint{prev_point.getLocationInGround()} : prev_point);
        }

        // emit remaining cylinders + spheres for fiber
        while (i < pps.size() && prev_traversal_position < fiber_end) {

            const OpenSim::AbstractGeometryPath::DecorativePathPoint& point = pps[i];
            const SimTK::Vec3 prev_to_pos = point.getLocationInGround() - prev_point.getLocationInGround();
            const auto prev_to_pos_len = static_cast<float>(prev_to_pos.norm());
            const float traversal_pos = prev_traversal_position + prev_to_pos_len;
            const float excess = traversal_pos - fiber_end;

            if (excess > 0.0f) {
                // emit end point and then exit
                const float scaler = (prev_to_pos_len - excess)/prev_to_pos_len;
                const SimTK::Vec3 fiber_end_pos = prev_point.getLocationInGround() + scaler * prev_to_pos;

                emit_fiber_cylinder(prev_point.getLocationInGround(), fiber_end_pos);
                emit_fiber_sphere(OpenSim::AbstractGeometryPath::DecorativePathPoint{fiber_end_pos});

                prev_point.setLocationInGround(fiber_end_pos);
                prev_traversal_position = fiber_end;
            }
            else {
                emit_fiber_cylinder(prev_point.getLocationInGround(), point.getLocationInGround());
                emit_fiber_sphere(point);

                i++;
                prev_point = point;
                prev_traversal_position = traversal_pos;
            }
        }

        // emit first sphere for second tendon
        if (i < pps.size()) {
            emit_tendon_sphere(OpenSim::AbstractGeometryPath::DecorativePathPoint{prev_point});
        }

        // emit remaining cylinders + spheres for second tendon
        while (i < pps.size()) {

            const OpenSim::AbstractGeometryPath::DecorativePathPoint& point = pps[i];
            const SimTK::Vec3 prev_to_pos = point.getLocationInGround() - prev_point.getLocationInGround();
            const auto prev_to_pos_len = static_cast<float>(prev_to_pos.norm());
            const float traversal_pos = prev_traversal_position + prev_to_pos_len;

            emit_tendon_cylinder(prev_point.getLocationInGround(), point.getLocationInGround());
            emit_tendon_sphere(point);

            i++;
            prev_point = point;
            prev_traversal_position = traversal_pos;
        }
    }

    // helper method: emits points (if required) and cylinders for a simple (no tendons)
    // point-based line (e.g. muscle or geometry path)
    void emit_point_based_line(
        RendererState& rs,
        const OpenSim::Component& hittest_target,
        std::span<const OpenSim::AbstractGeometryPath::DecorativePathPoint> points,
        float radius,
        const Color& color,
        SceneDecorationFlags flags)
    {
        if (points.empty()) {
            return;  // edge-case: there's no points to emit
        }

        // helper function: emits a sphere decoration
        const auto emit_sphere = [&rs, &hittest_target, radius, color, flags](
            const OpenSim::AbstractGeometryPath::DecorativePathPoint& pp,
            const Vector3& up_direction)
        {
            // ensure that user-defined path points are independently selectable (#425)
            const OpenSim::Component& c = pp.getAssociatedComponent() ?
                *pp.getAssociatedComponent() :
                hittest_target;

            rs.consume(c, SceneDecoration {
                .mesh = rs.sphere_mesh(),
                .transform = {
                    // ensure the sphere directionally tries to line up with the cylinders, to make
                    // the "join" between the sphere and cylinders nicer (#593)
                    .scale = Vector3{radius},
                    .rotation = normalize(rotation(Vector3{0.0f, 1.0f, 0.0f}, up_direction)),
                    .translation = to<Vector3>(pp.getLocationInGround())
                },
                .shading = color,
                .flags = flags,
            });
        };

        // helper function: emits a cylinder decoration between two points
        const auto emit_cylinder = [&rs, &hittest_target, radius, color, flags](
            const Vector3& p1,
            const Vector3& p2)
        {
            rs.consume(hittest_target, SceneDecoration{
                .mesh = rs.uncapped_cylinder_mesh(),
                .transform = cylinder_to_line_segment_transform({p1, p2}, radius),
                .shading  = color,
                .flags = flags,
            });
        };

        // if required, draw the first path point
        if (rs.get_show_path_points()) {
            const OpenSim::AbstractGeometryPath::DecorativePathPoint& first_point = points.front();
            const auto pp_pos = to<Vector3>(first_point.getLocationInGround());
            const Vector3 direction = points.size() == 1 ?
                Vector3{0.0f, 1.0f, 0.0f} :
                normalize(to<Vector3>(points[1].getLocationInGround()) - pp_pos);

            emit_sphere(first_point, direction);
        }

        // draw remaining cylinders and (if required) path points
        for (size_t i = 1; i < points.size(); ++i) {
            const OpenSim::AbstractGeometryPath::DecorativePathPoint& point = points[i];

            const Vector3& prev_pos = to<Vector3>(points[i - 1].getLocationInGround());
            const Vector3& cur_pos = to<Vector3>(point.getLocationInGround());

            emit_cylinder(prev_pos, cur_pos);

            // if required, draw path points
            if (rs.get_show_path_points()) {
                const Vector3 direction = normalize(cur_pos - prev_pos);
                emit_sphere(point, direction);
            }
        }
    }

    // OSC-specific decoration handler for "OpenSim-style" (line of action) decoration for an `OpenSim::Muscle`
    //
    // the reason this is used, rather than OpenSim's implementation, is because this custom implementation
    // can do things like recolor parts of the muscle, customize the hittest, etc.
    void handle_muscle_lines_of_action(
        RendererState& rs,
        const OpenSim::Muscle& musc)
    {
        const std::vector<OpenSim::AbstractGeometryPath::DecorativePathPoint> points =
            musc.getPath().getDecorativePathPoints(rs.get_state());

        const float radius = get_muscle_size(
            musc,
            rs.get_fixup_scale_factor(),
            rs.get_options().get_muscle_sizing_style()
        );

        emit_point_based_line(
            rs,
            musc,
            points,
            radius,
            rs.calc_muscle_color(musc),
            rs.calc_muscle_flags(musc)
        );
    }

    // custom implementation of `OpenSim::AbstractGeometryPath::generateDecorations`
    // that also handles tagging
    //
    // this specialized `OpenSim::AbstractGeometryPath` handler is used, rather than
    // `emit_generic_decorations`, because the custom implementation also coerces
    // selection hits to enable users to click on individual path points within
    // a path (#647)
    void handle_generic_geometry_path(
        RendererState& rs,
        const OpenSim::AbstractGeometryPath& gp,
        const OpenSim::Component& hittest_target)
    {
        const std::vector<OpenSim::AbstractGeometryPath::DecorativePathPoint> points =
            gp.getDecorativePathPoints(rs.get_state());
        const Color color = get_geometry_path_color(gp, rs.get_state());

        emit_point_based_line(
            rs,
            hittest_target,
            points,
            rs.get_fixup_scale_factor() * c_geometry_path_base_radius,
            color,
            rs.calc_geometry_path_flags(gp)
        );
    }

    void draw_line_of_action_arrow(
        RendererState& rs,
        const OpenSim::Muscle& muscle,
        const Ray& loa_point_direction,
        const Color& color)
    {
        const float fixup_scale_factor = rs.get_fixup_scale_factor();

        const ArrowProperties arrow_properties = {
            .start = loa_point_direction.origin,
            .end = loa_point_direction.origin + (fixup_scale_factor*0.1f)*loa_point_direction.direction,
            .tip_length = (fixup_scale_factor*0.015f),
            .neck_thickness = (fixup_scale_factor*0.006f),
            .head_thickness = (fixup_scale_factor*0.01f),
            .color = color,
            .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
        };
        draw_arrow(rs.upd_scene_cache(), arrow_properties, [&muscle, &rs](SceneDecoration&& d)
        {
            rs.consume(muscle, std::move(d));
        });
    }

    void handle_lines_of_action(
        RendererState& rs,
        const OpenSim::Muscle& musc)
    {
        // if options request, render effective muscle lines of action
        if (rs.get_options().get_should_show_effective_muscle_line_of_action_for_origin() or
            rs.get_options().get_should_show_effective_muscle_line_of_action_for_insertion()) {

            if (const auto loas = get_effective_lines_of_action_in_ground(musc, rs.get_state())) {

                if (rs.get_options().get_should_show_effective_muscle_line_of_action_for_origin()) {
                    draw_line_of_action_arrow(rs, musc, loas->origin, c_effective_line_of_action_color);
                }

                if (rs.get_options().get_should_show_effective_muscle_line_of_action_for_insertion()) {
                    draw_line_of_action_arrow(rs, musc, loas->insertion, c_effective_line_of_action_color);
                }
            }
        }

        // if options request, render anatomical muscle lines of action
        if (rs.get_options().get_should_show_anatomical_muscle_line_of_action_for_origin() or
            rs.get_options().get_should_show_anatomical_muscle_line_of_action_for_insertion()) {

            if (const auto loas = get_anatomical_lines_of_action_in_ground(musc, rs.get_state())) {

                if (rs.get_options().get_should_show_anatomical_muscle_line_of_action_for_origin()) {
                    draw_line_of_action_arrow(rs, musc, loas->origin, c_anatomical_line_of_action_color);
                }

                if (rs.get_options().get_should_show_anatomical_muscle_line_of_action_for_insertion()) {
                    draw_line_of_action_arrow(rs, musc, loas->insertion, c_anatomical_line_of_action_color);
                }
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::AbstractGeometryPath`
    void handle_geometry_path(
        RendererState& rs,
        const OpenSim::AbstractGeometryPath& gp)
    {
        if (not gp.get_Appearance().get_visible()) {
            // even custom muscle decoration implementations *must* obey the visibility
            // flag on `AbstractGeometryPath` (#414, #1166).
            return;
        }

        if (not gp.hasOwner()) {
            // it's a standalone path that's not part of a muscle
            handle_generic_geometry_path(rs, gp, gp);
            return;
        }

        // the `AbstractGeometryPath` has an owner, downcast to specialize
        if (const auto* const muscle = get_owner<OpenSim::Muscle>(gp)) {
            // owner is a muscle, coerce selection "hit" to the muscle

            handle_lines_of_action(rs, *muscle);

            switch (rs.get_options().get_muscle_decoration_style()) {
            case MuscleDecorationStyle::FibersAndTendons:
                handle_muscle_fibers_and_tendons(rs, *muscle);
                return;
            case MuscleDecorationStyle::Hidden:
                return;  // just don't generate them
            case MuscleDecorationStyle::LinesOfAction:
            default:
                handle_muscle_lines_of_action(rs, *muscle);
                return;
            }
        }
        else if (const auto* const ligament = get_owner<OpenSim::Ligament>(gp)) {
            // owner is an `OpenSim::Ligament`, coerce selection "hit" to the path actuator (#919)
            handle_generic_geometry_path(rs, gp, *ligament);
            return;
        }
        else if (const auto* const pa = get_owner<OpenSim::PathActuator>(gp)) {
            // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
            handle_generic_geometry_path(rs, gp, *pa);
            return;
        }
        else if (const auto* const path_spring = get_owner<OpenSim::PathSpring>(gp)) {
            // owner is a path spring, coerce selection "hit" to the path spring (#650)
            handle_generic_geometry_path(rs, gp, *path_spring);
            return;
        }
        else {
            // it's a path in some non-muscular context
            handle_generic_geometry_path(rs, gp, gp);
            return;
        }
    }

    void handle_frame_geometry(
        RendererState& rs,
        const OpenSim::FrameGeometry& frame_geometry)
    {
        // promote current component to the parent of the frame geometry, because
        // a user is probably more interested in the thing the frame geometry
        // represents (e.g. an offset frame) than the geometry itself (#506)
        const OpenSim::Component& component_to_link_to = get_owner_or(frame_geometry, frame_geometry);

        rs.emit_generic_decorations(frame_geometry, component_to_link_to);
    }

    void handle_hunt_crossley_force(
        RendererState& rs,
        const OpenSim::HuntCrossleyForce& hcf)
    {
        if (not rs.get_options().get_should_show_contact_forces()) {
            return;  // the user hasn't opted to see contact forces
        }

        // IGNORE: rs.get_model_display_hints().get_show_forces()
        //
        // because this is a user-enacted UI option and it would be silly
        // to expect the user to *also* toggle the "show_forces" option inside
        // the OpenSim model

        if (not hcf.appliesForce(rs.get_state())) {
            return;  // not applying this force
        }

        // else: try and compute a geometry-to-plane contact force and show it in-UI
        const std::optional<ForcePoint> contact_force_point = try_get_contact_force_in_ground(
            rs.get_model(),
            rs.get_state(),
            hcf
        );
        if (not contact_force_point) {
            return;
        }

        const float fixup_scale_factor = rs.get_fixup_scale_factor();
        const float len_scale = 0.0025f;
        const float base_radius = 0.025f;
        const float tip_length = 0.1f*length((fixup_scale_factor*len_scale)*contact_force_point->force);

        const ArrowProperties arrow_properties = {
            .start = contact_force_point->point,
            .end = contact_force_point->point + (fixup_scale_factor*len_scale)*contact_force_point->force,
            .tip_length = tip_length,
            .neck_thickness = fixup_scale_factor*base_radius*0.6f,
            .head_thickness = fixup_scale_factor*base_radius,
            .color = c_point_force_arrow_color,
            .decoration_flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
        };
        draw_arrow(rs.upd_scene_cache(), arrow_properties, [&hcf, &rs](SceneDecoration&& d)
        {
            rs.consume(hcf, std::move(d));
        });
    }

    void handle_scholz_geometry_path_obstacle(
        RendererState& rs,
        const OpenSim::Scholz2015GeometryPathObstacle& obstacle)
    {
        if (auto* sgp = dynamic_cast<const OpenSim::Scholz2015GeometryPath*>(&obstacle.getOwner())) {
            if (not sgp->get_Appearance().get_visible()) {
                // If the owning `Scholz2015GeometryPath` isn't visible, the
                // obstacles shouldn't be visible either (opensim-creator#1166).
                return;
            }
        }
        const SimTK::Vec3& contact_hint = obstacle.getContactHint();
        const SimTK::Vec3 contact_hint_in_ground = obstacle.getContactGeometry().getFrame().getTransformInGround(rs.get_state()) * obstacle.getContactGeometry().getTransform() * contact_hint;
        rs.consume(obstacle, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = {.scale = rs.get_fixup_scale_factor() * Vector3{0.01f}, .translation = to<Vector3>(contact_hint_in_ground)},
            .shading = Color::green(),
            .flags = {SceneDecorationFlag::AnnotationElement, SceneDecorationFlag::CanBackfaceCull},
        });
    }
}

void opyn::generate_model_decorations(
    SceneCache& mesh_cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts,
    float fixup_scale_factor,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out)
{
    generate_subcomponent_decorations(
        mesh_cache,
        model,
        state,
        model,  // i.e. the subcomponent is the root
        opts,
        fixup_scale_factor,
        out,
        false
    );
}

std::vector<SceneDecoration> opyn::generate_model_decorations(
    SceneCache& cache,
    const ModelStatePair& model_state,
    const OpenSimDecorationOptions& opts,
    float fixup_scale_factor)
{
    return generate_model_decorations(
        cache,
        model_state.get_model(),
        model_state.get_state(),
        opts,
        fixup_scale_factor
    );
}

std::vector<SceneDecoration> opyn::generate_model_decorations(
    SceneCache& cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts,
    float fixup_scale_factor)
{
    std::vector<SceneDecoration> rv;
    ComponentAbsPathDecorationTagger path_tagger;

    generate_subcomponent_decorations(
        cache,
        model,
        state,
        model,
        opts,
        fixup_scale_factor,
        [&rv, &path_tagger](const OpenSim::Component& component, SceneDecoration&& decoration)
        {
            path_tagger(component, decoration);
            rv.push_back(std::move(decoration));
        },
        false
    );
    return rv;
}

void opyn::generate_subcomponent_decorations(
    SceneCache& mesh_cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Component& subcomponent,
    const OpenSimDecorationOptions& opts,
    float fixup_scale_factor,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out,
    bool inclusive_of_provided_subcomponent)
{
    OSC_PERF("OpenSimRenderer/GenerateModelDecorations");

    RendererState renderer_state{
        mesh_cache,
        model,
        state,
        opts,
        fixup_scale_factor,
        out,
    };

    const auto emit_decorations_for_component = [&](const OpenSim::Component& c)
    {
        // handle OSC-specific decoration specializations, or fallback to generic
        // component decoration handling
        if (not should_show_in_ui(c)) {
            return;
        }
        else if (const auto* const custom = dynamic_cast<const CustomDecorationGenerator*>(&c)) {
            // edge-case: it's a component that has an OSC-specific `CustomDecorationGenerator`
            //            so we can skip the song-and-dance with caches, OpenSim, SimTK, etc.
            custom->generate_custom_decorations(renderer_state.get_state(), [&c, &renderer_state](SceneDecoration&& dec)
            {
                renderer_state.consume(c, std::move(dec));
            });
        }
        else if (const auto* const gp = dynamic_cast<const OpenSim::AbstractGeometryPath*>(&c)) {
            handle_geometry_path(renderer_state, *gp);
        }
        else if (const auto* const b = dynamic_cast<const OpenSim::Body*>(&c)) {
            handle_body(renderer_state, *b);
        }
        else if (const auto* const fg = dynamic_cast<const OpenSim::FrameGeometry*>(&c)) {
            handle_frame_geometry(renderer_state, *fg);
        }
        else if (const auto* const p2p = dynamic_cast<const OpenSim::PointToPointSpring*>(&c); p2p and opts.get_should_show_point_to_point_springs()) {
            generate_body_spatial_vector_arrow_decorations_for_forces_that_only_have_compute_force_method(renderer_state, *p2p);
            handle_point_to_point_spring(renderer_state, *p2p);
        }
        else if (typeid(c) == typeid(OpenSim::Station)) {
            // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            handle_station(renderer_state, dynamic_cast<const OpenSim::Station&>(c));
        }
        else if (const auto* const sj = dynamic_cast<const OpenSim::ScapulothoracicJoint*>(&c); sj && opts.get_should_show_scapulo()) {
            handle_scapulothoracic_joint(renderer_state, *sj);
        }
        else if (const auto* const hcf = dynamic_cast<const OpenSim::HuntCrossleyForce*>(&c)) {
            generate_body_spatial_vector_arrow_decorations_for_forces_that_only_have_compute_force_method(renderer_state, *hcf);
            handle_hunt_crossley_force(renderer_state, *hcf);
        }
        else if (dynamic_cast<const OpenSim::Geometry*>(&c)) {
            // EDGE-CASE:
            //
            // if the component being rendered is geometry that was explicitly added into the model then
            // the scene scale factor should not apply to that geometry
            renderer_state.emit_generic_decorations(c, c, 1.0f);  // note: override scale factor
        }
        else if (const auto* const force_producer = dynamic_cast<const OpenSim::ForceProducer*>(&c)) {
            generate_force_arrow_decorations_from_force_producer(renderer_state, *force_producer);
            renderer_state.emit_generic_decorations(c, c);
        }
        else if (const auto* const force = dynamic_cast<const OpenSim::Force*>(&c)) {
            generate_body_spatial_vector_arrow_decorations_for_forces_that_only_have_compute_force_method(renderer_state, *force);
            renderer_state.emit_generic_decorations(c, c);
        }
        else if (const auto* obstacle = dynamic_cast<const OpenSim::Scholz2015GeometryPathObstacle*>(&c); obstacle and opts.get_should_show_scholz2015_obstacle_contact_hints()) {
            handle_scholz_geometry_path_obstacle(renderer_state, *obstacle);
        }
        else {
            renderer_state.emit_generic_decorations(c, c);
        }
    };

    if (inclusive_of_provided_subcomponent) {
        emit_decorations_for_component(subcomponent);
    }
    for (const OpenSim::Component& c : subcomponent.getComponentList()) {
        emit_decorations_for_component(c);
    }
}

Mesh opyn::to_osc_mesh(
    SceneCache& mesh_cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh,
    const OpenSimDecorationOptions& opts,
    float fixup_scale_factor)
{
    std::vector<SceneDecoration> decs;
    decs.reserve(1);  // probable
    generate_subcomponent_decorations(
        mesh_cache,
        model,
        state,
        mesh,
        opts,
        fixup_scale_factor,
        [&decs](const OpenSim::Component&, SceneDecoration&& dec)
        {
            decs.push_back(std::move(dec));
        }
    );

    if (decs.empty()) {
        throw osc::formatted_runtime_error(
            "{}: could not be converted into an OSC mesh because OpenSim did not emit any decorations for the given OpenSim::Mesh component",
            mesh.getAbsolutePathString()
        );
    }
    if (decs.size() > 1) {
        log_warn("{}: this OpenSim::Mesh component generated more than one decoration: OSC defaulted to using the first one, but that may not be correct: if you are seeing unusual behavior, then it's because OpenSim is doing something wacky when generating decorations for a mesh",
            mesh.getAbsolutePathString()
        );
    }
    return std::move(decs.front().mesh);
}

Mesh opyn::to_osc_mesh(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh)
{
    SceneCache cache;
    const OpenSimDecorationOptions opts;
    return to_osc_mesh(cache, model, state, mesh, opts, 1.0f);
}

Mesh opyn::to_osc_mesh_bake_scale_factors(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh)
{
    Mesh rv = to_osc_mesh(model, state, mesh);
    rv.transform_vertices({.scale =  to<Vector3>(mesh.get_scale_factors())});

    return rv;
}

float opyn::get_recommended_scale_factor(
    SceneCache& mesh_cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts)
{
    // generate+union all scene decorations to get an idea of the scene size
    std::optional<AABB> aabb;
    generate_model_decorations(
        mesh_cache,
        model,
        state,
        opts,
        1.0f,
        [&aabb](const OpenSim::Component&, const SceneDecoration& dec)
        {
            aabb = bounding_aabb_of(aabb, dec.world_space_bounds());
        }
    );

    if (not aabb) {
        return 1.0f;  // no scene elements (the scene is empty)
    }

    // calculate the longest dimension and use that to figure out
    // what the smallest scale factor that would cause that dimension
    // to be >=1 cm (roughly the length of a frame leg in OSC's
    // decoration generator)
    float longest = rgs::max(dimensions_of(*aabb));
    float rv = 1.0f;
    while (longest < 0.01) {
        longest *= 10.0f;
        rv /= 10.0f;
    }

    return rv;
}
