#include "OpenSimDecorationGenerator.h"

#include <libopensimcreator/Documents/CustomComponents/ICustomDecorationGenerator.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <libopensimcreator/Graphics/ComponentSceneDecorationFlagsTagger.h>
#include <libopensimcreator/Graphics/OpenSimDecorationOptions.h>
#include <libopensimcreator/Graphics/SimTKDecorationGenerator.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/ClosedInterval.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/LineSegment.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/QuaternionFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/Perf.h>
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
#include <OpenSim/Simulation/Model/PointForceDirection.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;
namespace rgs = std::ranges;

namespace
{
    // constants
    inline constexpr float c_GeometryPathBaseRadius = 0.005f;
    inline constexpr float c_ForceArrowLengthScale = 0.0025f;
    inline constexpr float c_TorqueArrowLengthScale = 0.01f;
    inline constexpr Color c_EffectiveLineOfActionColor = Color::green();
    inline constexpr Color c_AnatomicalLineOfActionColor = Color::red();
    inline constexpr Color c_BodyForceArrowColor = Color::yellow();
    inline constexpr Color c_BodyTorqueArrowColor = Color::orange();
    inline constexpr Color c_PointForceArrowColor = Color::muted_yellow();  // note: should be similar to body force arrows
    inline constexpr Color c_StationColor = Color::red();
    inline constexpr Color c_ScapulothoracicJointColor =  Color::yellow().with_alpha(0.2f);
    inline constexpr Color c_CenterOfMassFirstColor = Color::lighter_grey();
    inline constexpr Color c_CenterOfMassSecondColor = Color::darker_grey();

    // helper: convert a physical frame's transform to ground into an Transform
    Transform TransformInGround(const OpenSim::Frame& frame, const SimTK::State& state)
    {
        return to<Transform>(frame.getTransformInGround(state));
    }

    Color GetGeometryPathDefaultColor(const OpenSim::GeometryPath& gp)
    {
        return Color{to<Vec3>(gp.getDefaultColor())};
    }

    Color GetGeometryPathColor(const OpenSim::GeometryPath& gp, const SimTK::State& st)
    {
        // returns the same color that OpenSim emits (which is usually just activation-based,
        // but might change in future versions of OpenSim)
        return Color{to<Vec3>(gp.getColor(st))};
    }

    // helper: calculates the radius of a muscle based on isometric force
    //
    // similar to how SCONE does it, so that users can compare between the two apps
    float GetSconeStyleAutomaticMuscleRadiusCalc(const OpenSim::Muscle& m)
    {
        const auto f = static_cast<float>(m.getMaxIsometricForce());
        const float specificTension = 0.25e6f;  // magic number?
        const float pcsa = f / specificTension;
        const float widthFactor = 0.25f;
        return widthFactor * sqrt(pcsa / pi_v<float>);
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(
        const OpenSim::Muscle& musc,
        float fixupScaleFactor,
        MuscleSizingStyle s)
    {
        switch (s) {
        case MuscleSizingStyle::PcsaDerived:
            return GetSconeStyleAutomaticMuscleRadiusCalc(musc) * fixupScaleFactor;
        case MuscleSizingStyle::Fixed:
        default:
            return c_GeometryPathBaseRadius * fixupScaleFactor;
        }
    }

    template<MuscleColorSource>
    float muscleColorSourceValueFor(const OpenSim::Muscle&, const SimTK::State&);
    template<>
    float muscleColorSourceValueFor<MuscleColorSource::Activation>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getActivation(state));
    }
    template<>
    float muscleColorSourceValueFor<MuscleColorSource::AppearanceProperty>(const OpenSim::Muscle&, const SimTK::State&)
    {
        return 1.0f;
    }
    template<>
    float muscleColorSourceValueFor<MuscleColorSource::Excitation>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getExcitation(state));
    }
    template<>
    float muscleColorSourceValueFor<MuscleColorSource::Force>(const OpenSim::Muscle& muscle, const SimTK::State& state)
    {
        return static_cast<float>(muscle.getActuation(state) / muscle.getMaxIsometricForce());
    }
    template<>
    float muscleColorSourceValueFor<MuscleColorSource::FiberLength>(const OpenSim::Muscle& muscle, const SimTK::State& state)
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
            MuscleColorSource colorSource,
            MuscleColorSourceScaling scaling) :

            m_Getter{chooseGetter(colorSource)},
            m_ScalingRange{chooseScalingRange(model, state, m_Getter, scaling)}
        {}

        // Returns a number in the range [0.0, 1.0] that describes the suggested position
        // a muscle's color should be on a color ramp (e.g. from blue to red).
        float lookup(const OpenSim::Muscle& muscle, const SimTK::State& state) const
        {
            const float v = m_Getter(muscle, state);
            const float t = m_ScalingRange.normalized_interpolant_at(v);
            return saturate(t);
        }

    private:
        using MuscleColorFactorGetter = float (*)(const OpenSim::Muscle&, const SimTK::State&);

        static MuscleColorFactorGetter chooseGetter(MuscleColorSource source)
        {
            switch (source) {
            case MuscleColorSource::AppearanceProperty: return muscleColorSourceValueFor<MuscleColorSource::AppearanceProperty>;
            case MuscleColorSource::Activation:         return muscleColorSourceValueFor<MuscleColorSource::Activation>;
            case MuscleColorSource::Excitation:         return muscleColorSourceValueFor<MuscleColorSource::Excitation>;
            case MuscleColorSource::Force:              return muscleColorSourceValueFor<MuscleColorSource::Force>;
            case MuscleColorSource::FiberLength:        return muscleColorSourceValueFor<MuscleColorSource::FiberLength>;
            default:                                    return muscleColorSourceValueFor<MuscleColorSource::Default>;
            }
        }

        static ClosedInterval<float> chooseScalingRange(
            const OpenSim::Model& model,
            const SimTK::State& state,
            const MuscleColorFactorGetter& getter,
            MuscleColorSourceScaling scaling)
        {
            static_assert(num_options<MuscleColorSourceScaling>() == 2);

            switch (scaling) {
            case MuscleColorSourceScaling::None:      return unit_interval<float>();
            case MuscleColorSourceScaling::ModelWide: return calculateModelWideScalingRange(model, state, getter);
            default:                                  return unit_interval<float>();
            }
        }

        static ClosedInterval<float> calculateModelWideScalingRange(
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

        MuscleColorFactorGetter m_Getter;
        ClosedInterval<float> m_ScalingRange;
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
            SceneCache& meshCache,
            const OpenSim::Model& model,
            const SimTK::State& state,
            const OpenSimDecorationOptions& opts,
            float fixupScaleFactor,
            const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out) :

            m_MeshCache{meshCache},
            m_Model{model},
            m_State{state},
            m_Opts{opts},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Out{out}
        {}

        SceneCache& updSceneCache()
        {
            return m_MeshCache;
        }

        const Mesh& sphere_mesh() const
        {
            return m_SphereMesh;
        }

        const Mesh& sphere_octant_mesh() const
        {
            return m_SphereOctantMesh;
        }

        const Mesh& uncapped_cylinder_mesh() const
        {
            return m_UncappedCylinderMesh;
        }

        const OpenSim::ModelDisplayHints& getModelDisplayHints() const
        {
            return m_ModelDisplayHints;
        }

        bool getShowPathPoints() const
        {
            return m_ShowPathPoints;
        }

        const SimTK::SimbodyMatterSubsystem& getMatterSubsystem() const
        {
            return m_MatterSubsystem;
        }

        const SimTK::State& getState() const
        {
            return m_State;
        }

        const OpenSimDecorationOptions& getOptions() const
        {
            return m_Opts;
        }

        const OpenSim::Model& getModel() const
        {
            return m_Model;
        }

        float getFixupScaleFactor() const
        {
            return m_FixupScaleFactor;
        }

        void consume(const OpenSim::Component& component, SceneDecoration&& dec)
        {
            // Filter out any scene decorations that have transforms that have any
            // NaN elements. This is a precaution to guard against bad maths in
            // OpenSim or OSC's custom decoration generator code (#976).
            if (any_element_is_nan(dec.transform)) {
                return;
            }
            m_Out(component, std::move(dec));
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emitGenericDecorations(
            const OpenSim::Component& componentToRender,
            const OpenSim::Component& componentToLinkTo,
            float fixupScaleFactor)
        {
            const std::function<void(SceneDecoration&&)> callback = [this, &componentToLinkTo](SceneDecoration&& dec)
            {
                consume(componentToLinkTo, std::move(dec));
            };

            m_GeomList.clear();
            componentToRender.generateDecorations(
                true,
                getModelDisplayHints(),
                getState(),
                m_GeomList
            );
            for (const SimTK::DecorativeGeometry& geom : m_GeomList)
            {
                GenerateDecorations(
                    updSceneCache(),
                    getMatterSubsystem(),
                    getState(),
                    geom,
                    fixupScaleFactor,
                    callback
                );
            }

            m_GeomList.clear();
            componentToRender.generateDecorations(
                false,
                getModelDisplayHints(),
                getState(),
                m_GeomList
            );
            for (const SimTK::DecorativeGeometry& geom : m_GeomList)
            {
                GenerateDecorations(
                    updSceneCache(),
                    getMatterSubsystem(),
                    getState(),
                    geom,
                    fixupScaleFactor,
                    callback
                );
            }
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emitGenericDecorations(
            const OpenSim::Component& componentToRender,
            const OpenSim::Component& componentToLinkTo)
        {
            emitGenericDecorations(componentToRender, componentToLinkTo, getFixupScaleFactor());
        }

        Color calcMuscleColor(const OpenSim::Muscle& muscle)
        {
            if (getOptions().getMuscleColorSource() == MuscleColorSource::AppearanceProperty) {
                // early-out: the muscle has a constant, Appearance-defined color
                return GetGeometryPathDefaultColor(muscle.getGeometryPath());
            }

            const float t = m_MuscleColorSourceScalingLookup.lookup(muscle, getState());
            const Color zeroColor = {50.0f / 255.0f, 50.0f / 255.0f, 166.0f / 255.0f};
            const Color fullColor = {255.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f};
            return lerp(zeroColor, fullColor, t);
        }
    private:
        SceneCache& m_MeshCache;
        Mesh m_SphereMesh = m_MeshCache.sphere_mesh();
        Mesh m_SphereOctantMesh = m_MeshCache.sphere_octant_mesh();
        Mesh m_UncappedCylinderMesh = m_MeshCache.uncapped_cylinder_mesh();
        const OpenSim::Model& m_Model;
        const OpenSim::ModelDisplayHints& m_ModelDisplayHints = m_Model.getDisplayHints();
        bool m_ShowPathPoints = m_ModelDisplayHints.get_show_path_points();
        const SimTK::SimbodyMatterSubsystem& m_MatterSubsystem = m_Model.getSystem().getMatterSubsystem();
        const SimTK::State& m_State;
        const OpenSimDecorationOptions& m_Opts;
        float m_FixupScaleFactor;
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& m_Out;
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomList;
        MuscleColorFactorLookup m_MuscleColorSourceScalingLookup{m_Model, m_State, m_Opts.getMuscleColorSource(), m_Opts.getMuscleColorSourceScaling()};
    };

    // An `OpenSim::ForceConsumer` that emits `SceneDecoration` arrows that represent
    // each force vector it has consumed.
    //
    // Callers should also call `emitAccumulatedBodySpatialVecs` after `produceForces`
    // has completed, because this implementation automatically merges body forces on
    // the same body together.
    class SceneDecorationGeneratingForceConsumer : public OpenSim::ForceConsumer {
    public:
        explicit SceneDecorationGeneratingForceConsumer(
            RendererState* rendererState,
            const OpenSim::ForceProducer* forceProducer) :
            m_RendererState{rendererState},
            m_AssociatedForceProducer{forceProducer}
        {}

        // Emit any body forces that were accumulated during the production phase
        void emitAccumulatedBodySpatialVecs(const SimTK::State& state)
        {
            for (const auto& [bodyPtr, spatialVec] : m_AccumulatedBodySpatialVecs) {
                handleBodyTorque(state, *bodyPtr, spatialVec[0]);
                handleBodyForce(state, *bodyPtr, spatialVec[1]);
            }
        }
    private:
        // Implements `ForceConsumer` API by generating equivalent `SceneDecoration`s for
        // the body torque + force (separately).
        void implConsumeBodySpatialVec(
            const SimTK::State&,
            const OpenSim::PhysicalFrame& body,
            const SimTK::SpatialVec& spatialVec) override
        {
            if (m_AccumulatedBodySpatialVecs.empty()) {
                // Lazily reserve memory for the accumulated body forces lookup. Most
                // `ForceProducer`s will only touch a few `Body`s, 8 is a guess on the
                // most likely upper limit.
                m_AccumulatedBodySpatialVecs.reserve(8);
            }

            // Accumulate the body forces, rather than emitting them seperately, because
            // it makes the visualization less cluttered.
            auto& accumulator = m_AccumulatedBodySpatialVecs.try_emplace(&body, SimTK::SpatialVec{SimTK::Vec3{0.0}, SimTK::Vec3{0.0}}).first->second;
            accumulator += spatialVec;
        }

        // Implements `ForceConsumer` API by generating equivalent `SceneDecoration`s for
        // the point force (incl. conversion to a resultant body force)
        void implConsumePointForce(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& frame,
            const SimTK::Vec3& point,
            const SimTK::Vec3& forceInGround) override
        {
            if (equal_within_scaled_epsilon(forceInGround.normSqr(), 0.0)) {
                return;  // zero/small force provided: skip it
            }

            // if requested, generate an arrow decoration for the point force
            if (m_RendererState->getOptions().getShouldShowPointForces()) {
                const float fixupScaleFactor = m_RendererState->getFixupScaleFactor();
                const SimTK::Vec3 positionInGround = frame.findStationLocationInGround(state, point);
                const ArrowProperties arrowProperties = {
                    .start = to<Vec3>(positionInGround),
                    .end = to<Vec3>(positionInGround + (fixupScaleFactor * c_ForceArrowLengthScale * forceInGround)),
                    .tip_length = 0.015f * fixupScaleFactor,
                    .neck_thickness = 0.006f * fixupScaleFactor,
                    .head_thickness = 0.01f * fixupScaleFactor,
                    .color = c_PointForceArrowColor,
                    .decoration_flags = SceneDecorationFlag::AnnotationElement,
                };

                draw_arrow(m_RendererState->updSceneCache(), arrowProperties, [this](SceneDecoration&& decoration)
                {
                    m_RendererState->consume(*m_AssociatedForceProducer, std::move(decoration));
                });
            }

            // accumulate associated body force
            {
                // maths ripped from `SimbodyMatterSubsystem::addInStationForce`
                //
                // https://github.com/simbody/simbody/blob/34b0ac47e6252457733a503c234b2daf1c596d81/Simbody/src/SimbodyMatterSubsystem.cpp#L2190

                const auto& baseFrame = dynamic_cast<const OpenSim::PhysicalFrame&>(frame.findBaseFrame());
                const SimTK::Rotation& R_GB = baseFrame.getTransformInGround(state).R();
                const SimTK::Vec3 torque = (R_GB * point) % forceInGround;
                implConsumeBodySpatialVec(state, baseFrame, SimTK::SpatialVec{torque, forceInGround});
            }
        }

        // Helper method for drawing the torque part of a `SimTK::SpatialVec`
        void handleBodyTorque(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& body,
            const SimTK::Vec3& torqueInGround)
        {
            if (not m_RendererState->getOptions().getShouldShowForceAngularComponent()) {
                return;  // the caller has opted out of showing torques on bodies
            }
            if (equal_within_scaled_epsilon(torqueInGround.normSqr(), 0.0)) {
                return;  // zero/small torque provided: skip it
            }

            const float fixupScaleFactor = m_RendererState->getFixupScaleFactor();
            const SimTK::Transform& frame2ground = body.getTransformInGround(state);
            const ArrowProperties arrowProperties = {
                .start = to<Vec3>(frame2ground * SimTK::Vec3{0.0}),
                .end = to<Vec3>(frame2ground * (fixupScaleFactor * c_TorqueArrowLengthScale * torqueInGround)),
                .tip_length = (fixupScaleFactor*0.015f),
                .neck_thickness = (fixupScaleFactor*0.006f),
                .head_thickness = (fixupScaleFactor*0.01f),
                .color = c_BodyTorqueArrowColor,
                .decoration_flags = SceneDecorationFlag::AnnotationElement,
            };
            draw_arrow(m_RendererState->updSceneCache(), arrowProperties, [this](SceneDecoration&& decoration)
            {
                m_RendererState->consume(*m_AssociatedForceProducer, std::move(decoration));
            });
        }

        // Helper method for drawing the force part of a `SimTK::SpatialVec`
        void handleBodyForce(
            const SimTK::State& state,
            const OpenSim::PhysicalFrame& body,
            const SimTK::Vec3& forceInGround)
        {
            if (not m_RendererState->getOptions().getShouldShowForceLinearComponent()) {
                return;  // the caller has opted out of showing forces on bodies
            }
            if (equal_within_scaled_epsilon(forceInGround.normSqr(), 0.0)) {
                return;  // zero/small force provided: skip it
            }

            const float fixupScaleFactor = m_RendererState->getFixupScaleFactor();
            const SimTK::Transform& frame2ground = body.getTransformInGround(state);
            const ArrowProperties arrowProperties = {
                .start = to<Vec3>(frame2ground.p()),
                .end =  to<Vec3>(frame2ground.p() + (fixupScaleFactor * c_ForceArrowLengthScale * forceInGround)),
                .tip_length = (fixupScaleFactor*0.015f),
                .neck_thickness = (fixupScaleFactor*0.006f),
                .head_thickness = (fixupScaleFactor*0.01f),
                .color = c_BodyForceArrowColor,
                .decoration_flags = SceneDecorationFlag::AnnotationElement,
            };
            draw_arrow(m_RendererState->updSceneCache(), arrowProperties, [this](SceneDecoration&& decoration)
            {
                m_RendererState->consume(*m_AssociatedForceProducer, std::move(decoration));
            });
        }

        RendererState* m_RendererState;
        OpenSim::ForceProducer const* m_AssociatedForceProducer;
        std::unordered_map<const OpenSim::PhysicalFrame*, SimTK::SpatialVec> m_AccumulatedBodySpatialVecs;
    };

    // OSC-specific decoration handler that decorates the body forces/torques applied by
    // an `OpenSim::Force` using the `OpenSim::Force::computeForce` API
    //
    // Note: if an `OpenSim::Force` is actually an `OpenSim::ForceProducer`, then use that
    //       API instead - this code is here to support "legacy" forces that haven't
    //       implemented that API yet. An overview of the `ForceProducer` API explains the
    //       relevant motivations etc: https://github.com/opensim-org/opensim-core/pull/3891
    void GenerateBodySpatialVectorArrowDecorationsForForcesThatOnlyHaveComputeForceMethod(
        RendererState& rs,
        const OpenSim::Force& force)
    {
        const bool showForces = rs.getOptions().getShouldShowForceLinearComponent();
        const bool showTorques = rs.getOptions().getShouldShowForceAngularComponent();
        if (not showForces and not showTorques) {
            return;  // caller doesn't want to draw this
        }

        if (not force.appliesForce(rs.getState())) {
            return;  // the `Force` does not apply a force
        }

        // this is a very heavy-handed way of getting the relevant information, because
        // OpenSim's `Force` implementation implicitly assumes that all body forces are
        // available in one contiguous vector

        const SimTK::SimbodyMatterSubsystem& matter = rs.getMatterSubsystem();
        const SimTK::State& state = rs.getState();

        const OpenSim::ForceAdapter adapter{force};
        SimTK::Vector_<SimTK::SpatialVec> bodyForces(matter.getNumBodies(), SimTK::SpatialVec{SimTK::Vec3{0.0}, SimTK::Vec3{0.0}});
        SimTK::Vector_<SimTK::Vec3> particleForces(matter.getNumParticles(), SimTK::Vec3{0.0});  // (unused)
        SimTK::Vector mobilityForces(matter.getNumMobilities(), double{});  // (unused)

        adapter.calcForce(
            state,
            bodyForces,
            particleForces,  // unused, but required
            mobilityForces   // unused, but required
        );

        const float fixupScaleFactor = rs.getFixupScaleFactor();
        for (SimTK::MobilizedBodyIndex bodyIdx{0}; bodyIdx < bodyForces.size(); ++bodyIdx) {

            const SimTK::MobilizedBody& mobod = matter.getMobilizedBody(bodyIdx);
            const SimTK::Transform mobod2ground = mobod.getBodyTransform(state);

            // if applicable, handle drawing the linear component of force as an arrow
            if (showForces) {
                const SimTK::Vec3 forceVec = bodyForces[bodyIdx][1];
                if (equal_within_scaled_epsilon(forceVec.normSqr(), 0.0)) {
                    continue;  // no translational force applied
                }

                const ArrowProperties arrowProperties = {
                    .start = to<Vec3>(mobod2ground.p()),
                    .end = to<Vec3>(mobod2ground.p() + (fixupScaleFactor * c_ForceArrowLengthScale * forceVec)),
                    .tip_length = (fixupScaleFactor*0.015f),
                    .neck_thickness = (fixupScaleFactor*0.006f),
                    .head_thickness = (fixupScaleFactor*0.01f),
                    .color = c_BodyForceArrowColor,
                    .decoration_flags = SceneDecorationFlag::AnnotationElement,
                };
                draw_arrow(rs.updSceneCache(), arrowProperties, [&force, &rs](SceneDecoration&& decoration)
                {
                    rs.consume(force, std::move(decoration));
                });
            }

            // if applicable, handle drawing the angular component of force as an arrow
            if (showTorques) {
                const SimTK::Vec3 torqueVec = bodyForces[bodyIdx][0];
                if (equal_within_scaled_epsilon(torqueVec.normSqr(), 0.0)) {
                    continue;  // no translational force applied
                }

                const ArrowProperties arrowProperties = {
                    .start = to<Vec3>(mobod2ground * SimTK::Vec3{0.0}),
                    .end = to<Vec3>(mobod2ground * (fixupScaleFactor * c_TorqueArrowLengthScale * torqueVec)),
                    .tip_length = (fixupScaleFactor*0.015f),
                    .neck_thickness = (fixupScaleFactor*0.006f),
                    .head_thickness = (fixupScaleFactor*0.01f),
                    .color = c_BodyTorqueArrowColor,
                    .decoration_flags = SceneDecorationFlag::AnnotationElement,
                };
                draw_arrow(rs.updSceneCache(), arrowProperties, [&force, &rs](SceneDecoration&& decoration)
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
    void GenerateForceArrowDecorationsFromForceProducer(
        RendererState& rs,
        const OpenSim::ForceProducer& forceProducer)
    {
        if (not forceProducer.appliesForce(rs.getState())) {
            return;  // the `ForceProducer` is currently disabled
        }

        if (not rs.getOptions().getShouldShowPointForces() and
            not rs.getOptions().getShouldShowForceLinearComponent() and
            not rs.getOptions().getShouldShowForceAngularComponent()) {

            return;  // caller doesn't want to draw any kind of force vector
        }

        SceneDecorationGeneratingForceConsumer consumer{&rs, &forceProducer};
        forceProducer.produceForces(rs.getState(), consumer);
        consumer.emitAccumulatedBodySpatialVecs(rs.getState());
    }

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void HandlePointToPointSpring(
        RendererState& rs,
        const OpenSim::PointToPointSpring& p2p)
    {
        if (not rs.getOptions().getShouldShowPointToPointSprings()) {
            return;
        }

        const Vec3 p1 = TransformInGround(p2p.getBody1(), rs.getState()) * to<Vec3>(p2p.getPoint1());
        const Vec3 p2 = TransformInGround(p2p.getBody2(), rs.getState()) * to<Vec3>(p2p.getPoint2());

        const float radius = c_GeometryPathBaseRadius * rs.getFixupScaleFactor();

        rs.consume(p2p, SceneDecoration{
            .mesh = rs.updSceneCache().cylinder_mesh(),
            .transform = cylinder_to_line_segment_transform({p1, p2}, radius),
            .shading = Color::light_grey(),
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(
        RendererState& rs,
        const OpenSim::Station& s)
    {
        const float radius = rs.getFixupScaleFactor() * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        rs.consume(s, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = {
                .scale = Vec3{radius},
                .translation = to<Vec3>(s.getLocationInGround(rs.getState())),
            },
            .shading = c_StationColor,
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void HandleScapulothoracicJoint(
        RendererState& rs,
        const OpenSim::ScapulothoracicJoint& scapuloJoint)
    {
        Transform t = TransformInGround(scapuloJoint.getParentFrame(), rs.getState());
        t.scale = to<Vec3>(scapuloJoint.get_thoracic_ellipsoid_radii_x_y_z());

        rs.consume(scapuloJoint, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = t,
            .shading = c_ScapulothoracicJointColor,
        });
    }

    // OSC-specific decoration handler for body centers of mass
    void HandleBodyCentersOfMass(
        RendererState& rs,
        const OpenSim::Body& b)
    {
        if (not rs.getOptions().getShouldShowCentersOfMass()) {
            return;
        }
        if (b.getMassCenter() == SimTK::Vec3{0.0}) {
            return;
        }

        // draw a COM by drawing 8 sphere octants to form a sphere
        // with two alternating colors (standard visual notation used
        // by engineers etc.)

        const float radius = rs.getFixupScaleFactor() * 0.0075f;
        Transform t = TransformInGround(b, rs.getState());
        t.translation = t * to<Vec3>(b.getMassCenter());
        t.scale = Vec3{radius};

        // draw four octants with the first color
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t,
            .shading = c_CenterOfMassFirstColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::x())),
            .shading = c_CenterOfMassFirstColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::y())),
            .shading = c_CenterOfMassFirstColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::z())),
            .shading = c_CenterOfMassFirstColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });

        // draw four octants with the second color
        t.scale.x *= -1.0f;  // mirror image along one plane
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t,
            .shading = c_CenterOfMassSecondColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::x())),
            .shading = c_CenterOfMassSecondColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::y())),
            .shading = c_CenterOfMassSecondColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_octant_mesh(),
            .transform = t.with_rotation(t.rotation * angle_axis(180_deg, CoordinateDirection::z())),
            .shading = c_CenterOfMassSecondColor,
            .flags = SceneDecorationFlag::AnnotationElement,
        });
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(
        RendererState& rs,
        const OpenSim::Body& b)
    {
        HandleBodyCentersOfMass(rs, b);  // CoMs are handled by OSC
        rs.emitGenericDecorations(b, b);  // bodies are emitted by OpenSim
    }

    // OSC-specific decoration handler for Muscle+Fiber representation of an `OpenSim::Muscle`
    void HandleMuscleFibersAndTendons(
        RendererState& rs,
        const OpenSim::Muscle& muscle)
    {
        const std::vector<GeometryPathPoint> pps = GetAllPathPoints(muscle.getGeometryPath(), rs.getState());
        if (pps.empty()) {
            return;  // edge-case: there are no points in the muscle path
        }

        // precompute various coefficients, reused meshes, helpers, etc.

        const float fixupScaleFactor = rs.getFixupScaleFactor();

        const float fiberUiRadius = GetMuscleSize(
            muscle,
            fixupScaleFactor,
            rs.getOptions().getMuscleSizingStyle()
        );
        const float tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        const Color fiberColor = rs.calcMuscleColor(muscle);
        const Color tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f};

        const SceneDecoration tendonSpherePrototype = {
            .mesh = rs.sphere_mesh(),
            .transform = {.scale = Vec3{tendonUiRadius}},
            .shading = tendonColor,
        };
        const SceneDecoration tendonCylinderPrototype = {
            .mesh = rs.uncapped_cylinder_mesh(),
            .shading = tendonColor,
        };
        const SceneDecoration fiberSpherePrototype = {
            .mesh = rs.sphere_mesh(),
            .transform = {.scale = Vec3{fiberUiRadius}},
            .shading = fiberColor,
        };
        const SceneDecoration fiberCylinderPrototype = {
            .mesh = rs.uncapped_cylinder_mesh(),
            .shading = fiberColor,
        };

        const auto emitTendonSphere = [&](const GeometryPathPoint& p)
        {
            const OpenSim::Component* c = &muscle;
            if (p.maybeUnderlyingUserPathPoint) {
                c = p.maybeUnderlyingUserPathPoint;
            }
            rs.consume(*c, tendonSpherePrototype.with_translation(p.locationInGround));
        };
        const auto emitTendonCylinder = [&](const Vec3& p1, const Vec3& p2)
        {
            const Transform xform = cylinder_to_line_segment_transform({p1, p2}, tendonUiRadius);
            rs.consume(muscle, tendonCylinderPrototype.with_transform(xform));
        };
        auto emitFiberSphere = [&](const GeometryPathPoint& p)
        {
            const OpenSim::Component* c = &muscle;
            if (p.maybeUnderlyingUserPathPoint) {
                c = p.maybeUnderlyingUserPathPoint;
            }
            rs.consume(*c, fiberSpherePrototype.with_translation(p.locationInGround));
        };
        auto emitFiberCylinder = [&](const Vec3& p1, const Vec3& p2)
        {
            const Transform xform = cylinder_to_line_segment_transform({p1, p2}, fiberUiRadius);
            rs.consume(muscle, fiberCylinderPrototype.with_transform(xform));
        };

        // start emitting the path

        if (pps.size() == 1) {
            // edge-case: this shouldn't happen but, just to be safe...
            emitFiberSphere(pps.front());
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it and split it into tendon-fiber-tendon
        const float tendonLen = max(0.0f, static_cast<float>(muscle.getTendonLength(rs.getState()) * 0.5));
        const float fiberLen = max(0.0f, static_cast<float>(muscle.getFiberLength(rs.getState())));
        const float fiberEnd = tendonLen + fiberLen;
        const bool hasTendonSpheres = tendonLen > 0.0f;

        size_t i = 1;
        GeometryPathPoint prevPoint = pps.front();
        float prevTraversalPosition = 0.0f;

        // emit first sphere for first tendon
        if (prevTraversalPosition < tendonLen) {
            emitTendonSphere(prevPoint);  // emit first tendon sphere
        }

        // emit remaining cylinders + spheres for first tendon
        while (i < pps.size() && prevTraversalPosition < tendonLen) {

            const GeometryPathPoint& point = pps[i];
            const Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            const float prevToPosLen = length(prevToPos);
            const float traversalPos = prevTraversalPosition + prevToPosLen;
            const float excess = traversalPos - tendonLen;

            if (excess > 0.0f) {
                const float scaler = (prevToPosLen - excess)/prevToPosLen;
                const Vec3 tendonEnd = prevPoint.locationInGround + scaler * prevToPos;

                emitTendonCylinder(prevPoint.locationInGround, tendonEnd);
                emitTendonSphere(GeometryPathPoint{tendonEnd});

                prevPoint.locationInGround = tendonEnd;
                prevTraversalPosition = tendonLen;
            }
            else {
                emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
                emitTendonSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPosition = traversalPos;
            }
        }

        // emit first sphere for fiber
        if (i < pps.size() && prevTraversalPosition < fiberEnd) {
            // label the sphere if no tendon spheres were previously emitted
            emitFiberSphere(hasTendonSpheres ? GeometryPathPoint{prevPoint.locationInGround} : prevPoint);
        }

        // emit remaining cylinders + spheres for fiber
        while (i < pps.size() && prevTraversalPosition < fiberEnd) {

            const GeometryPathPoint& point = pps[i];
            const Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            const float prevToPosLen = length(prevToPos);
            const float traversalPos = prevTraversalPosition + prevToPosLen;
            const float excess = traversalPos - fiberEnd;

            if (excess > 0.0f) {
                // emit end point and then exit
                const float scaler = (prevToPosLen - excess)/prevToPosLen;
                const Vec3 fiberEndPos = prevPoint.locationInGround + scaler * prevToPos;

                emitFiberCylinder(prevPoint.locationInGround, fiberEndPos);
                emitFiberSphere(GeometryPathPoint{fiberEndPos});

                prevPoint.locationInGround = fiberEndPos;
                prevTraversalPosition = fiberEnd;
            }
            else {
                emitFiberCylinder(prevPoint.locationInGround, point.locationInGround);
                emitFiberSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPosition = traversalPos;
            }
        }

        // emit first sphere for second tendon
        if (i < pps.size()) {
            emitTendonSphere(GeometryPathPoint{prevPoint});
        }

        // emit remaining cylinders + spheres for second tendon
        while (i < pps.size()) {

            const GeometryPathPoint& point = pps[i];
            const Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            const float prevToPosLen = length(prevToPos);
            const float traversalPos = prevTraversalPosition + prevToPosLen;

            emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
            emitTendonSphere(point);

            i++;
            prevPoint = point;
            prevTraversalPosition = traversalPos;
        }
    }

    // helper method: emits points (if required) and cylinders for a simple (no tendons)
    // point-based line (e.g. muscle or geometry path)
    void EmitPointBasedLine(
        RendererState& rs,
        const OpenSim::Component& hittestTarget,
        std::span<const GeometryPathPoint> points,
        float radius,
        const Color& color)
    {
        if (points.empty()) {
            return;  // edge-case: there's no points to emit
        }

        // helper function: emits a sphere decoration
        const auto emitSphere = [&rs, &hittestTarget, radius, color](
            const GeometryPathPoint& pp,
            const Vec3& upDirection)
        {
            // ensure that user-defined path points are independently selectable (#425)
            const OpenSim::Component& c = pp.maybeUnderlyingUserPathPoint ?
                *pp.maybeUnderlyingUserPathPoint :
                hittestTarget;

            rs.consume(c, SceneDecoration {
                .mesh = rs.sphere_mesh(),
                .transform = {
                    // ensure the sphere directionally tries to line up with the cylinders, to make
                    // the "join" between the sphere and cylinders nicer (#593)
                    .scale = Vec3{radius},
                    .rotation = normalize(rotation(Vec3{0.0f, 1.0f, 0.0f}, upDirection)),
                    .translation = pp.locationInGround
                },
                .shading = color,
            });
        };

        // helper function: emits a cylinder decoration between two points
        const auto emitCylinder = [&rs, &hittestTarget, radius, color](
            const Vec3& p1,
            const Vec3& p2)
        {
            rs.consume(hittestTarget, SceneDecoration{
                .mesh = rs.uncapped_cylinder_mesh(),
                .transform = cylinder_to_line_segment_transform({p1, p2}, radius),
                .shading  = color,
            });
        };

        // if required, draw the first path point
        if (rs.getShowPathPoints()) {
            const GeometryPathPoint& firstPoint = points.front();
            const Vec3& ppPos = firstPoint.locationInGround;
            const Vec3 direction = points.size() == 1 ?
                Vec3{0.0f, 1.0f, 0.0f} :
                normalize(points[1].locationInGround - ppPos);

            emitSphere(firstPoint, direction);
        }

        // draw remaining cylinders and (if required) path points
        for (size_t i = 1; i < points.size(); ++i) {
            const GeometryPathPoint& point = points[i];

            const Vec3& prevPos = points[i - 1].locationInGround;
            const Vec3& curPos = point.locationInGround;

            emitCylinder(prevPos, curPos);

            // if required, draw path points
            if (rs.getShowPathPoints()) {
                const Vec3 direction = normalize(curPos - prevPos);
                emitSphere(point, direction);
            }
        }
    }

    // OSC-specific decoration handler for "OpenSim-style" (line of action) decoration for an `OpenSim::Muscle`
    //
    // the reason this is used, rather than OpenSim's implementation, is because this custom implementation
    // can do things like recolor parts of the muscle, customize the hittest, etc.
    void HandleMuscleLinesOfAction(
        RendererState& rs,
        const OpenSim::Muscle& musc)
    {
        const std::vector<GeometryPathPoint> points =
            GetAllPathPoints(musc.getGeometryPath(), rs.getState());

        const float radius = GetMuscleSize(
            musc,
            rs.getFixupScaleFactor(),
            rs.getOptions().getMuscleSizingStyle()
        );

        const Color color = rs.calcMuscleColor(musc);

        EmitPointBasedLine(rs, musc, points, radius, color);
    }

    // custom implementation of `OpenSim::GeometryPath::generateDecorations` that also
    // handles tagging
    //
    // this specialized `OpenSim::GeometryPath` handler is used, rather than
    // `emitGenericDecorations`, because the custom implementation also coerces
    // selection hits to enable users to click on individual path points within
    // a path (#647)
    void HandleGenericGeometryPath(
        RendererState& rs,
        const OpenSim::GeometryPath& gp,
        const OpenSim::Component& hittestTarget)
    {
        const std::vector<GeometryPathPoint> points = GetAllPathPoints(gp, rs.getState());
        const Color color = GetGeometryPathColor(gp, rs.getState());

        EmitPointBasedLine(
            rs,
            hittestTarget,
            points,
            rs.getFixupScaleFactor() * c_GeometryPathBaseRadius,
            color
        );
    }

    void DrawLineOfActionArrow(
        RendererState& rs,
        const OpenSim::Muscle& muscle,
        const PointDirection& loaPointDirection,
        const Color& color)
    {
        const float fixupScaleFactor = rs.getFixupScaleFactor();

        const ArrowProperties arrowProperties = {
            .start = loaPointDirection.point,
            .end = loaPointDirection.point + (fixupScaleFactor*0.1f)*loaPointDirection.direction,
            .tip_length = (fixupScaleFactor*0.015f),
            .neck_thickness = (fixupScaleFactor*0.006f),
            .head_thickness = (fixupScaleFactor*0.01f),
            .color = color,
            .decoration_flags = SceneDecorationFlag::AnnotationElement,
        };
        draw_arrow(rs.updSceneCache(), arrowProperties, [&muscle, &rs](SceneDecoration&& d)
        {
            rs.consume(muscle, std::move(d));
        });
    }

    void HandleLinesOfAction(
        RendererState& rs,
        const OpenSim::Muscle& musc)
    {
        // if options request, render effective muscle lines of action
        if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin() or
            rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion()) {

            if (const auto loas = GetEffectiveLinesOfActionInGround(musc, rs.getState())) {

                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin()) {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_EffectiveLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion()) {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_EffectiveLineOfActionColor);
                }
            }
        }

        // if options request, render anatomical muscle lines of action
        if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin() or
            rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion()) {

            if (const auto loas = GetAnatomicalLinesOfActionInGround(musc, rs.getState())) {

                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin()) {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_AnatomicalLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion()) {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_AnatomicalLineOfActionColor);
                }
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(
        RendererState& rs,
        const OpenSim::GeometryPath& gp)
    {
        if (not gp.get_Appearance().get_visible()) {
            // even custom muscle decoration implementations *must* obey the visibility
            // flag on `GeometryPath` (#414)
            return;
        }

        if (not gp.hasOwner()) {
            // it's a standalone path that's not part of a muscle
            HandleGenericGeometryPath(rs, gp, gp);
            return;
        }

        // the `GeometryPath` has a owner, downcast to specialize
        if (const auto* const muscle = GetOwner<OpenSim::Muscle>(gp)) {
            // owner is a muscle, coerce selection "hit" to the muscle

            HandleLinesOfAction(rs, *muscle);

            switch (rs.getOptions().getMuscleDecorationStyle()) {
            case MuscleDecorationStyle::FibersAndTendons:
                HandleMuscleFibersAndTendons(rs, *muscle);
                return;
            case MuscleDecorationStyle::Hidden:
                return;  // just don't generate them
            case MuscleDecorationStyle::LinesOfAction:
            default:
                HandleMuscleLinesOfAction(rs, *muscle);
                return;
            }
        }
        else if (const auto* const ligament = GetOwner<OpenSim::Ligament>(gp)) {
            // owner is an `OpenSim::Ligament`, coerce selection "hit" to the path actuator (#919)
            HandleGenericGeometryPath(rs, gp, *ligament);
            return;
        }
        else if (const auto* const pa = GetOwner<OpenSim::PathActuator>(gp)) {
            // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
            HandleGenericGeometryPath(rs, gp, *pa);
            return;
        }
        else if (const auto* const pathSpring = GetOwner<OpenSim::PathSpring>(gp)) {
            // owner is a path spring, coerce selection "hit" to the path spring (#650)
            HandleGenericGeometryPath(rs, gp, *pathSpring);
            return;
        }
        else {
            // it's a path in some non-muscular context
            HandleGenericGeometryPath(rs, gp, gp);
            return;
        }
    }

    void HandleFrameGeometry(
        RendererState& rs,
        const OpenSim::FrameGeometry& frameGeometry)
    {
        // promote current component to the parent of the frame geometry, because
        // a user is probably more interested in the thing the frame geometry
        // represents (e.g. an offset frame) than the geometry itself (#506)
        const OpenSim::Component& componentToLinkTo = GetOwnerOr(frameGeometry, frameGeometry);

        rs.emitGenericDecorations(frameGeometry, componentToLinkTo);
    }

    void HandleHuntCrossleyForce(
        RendererState& rs,
        const OpenSim::HuntCrossleyForce& hcf)
    {
        if (not rs.getOptions().getShouldShowContactForces()) {
            return;  // the user hasn't opted to see contact forces
        }

        // IGNORE: rs.getModelDisplayHints().get_show_forces()
        //
        // because this is a user-enacted UI option and it would be silly
        // to expect the user to *also* toggle the "show_forces" option inside
        // the OpenSim model

        if (not hcf.appliesForce(rs.getState())) {
            return;  // not applying this force
        }

        // else: try and compute a geometry-to-plane contact force and show it in-UI
        const std::optional<ForcePoint> contactForcePoint = TryGetContactForceInGround(
            rs.getModel(),
            rs.getState(),
            hcf
        );
        if (not contactForcePoint) {
            return;
        }

        const float fixupScaleFactor = rs.getFixupScaleFactor();
        const float lenScale = 0.0025f;
        const float baseRadius = 0.025f;
        const float tip_length = 0.1f*length((fixupScaleFactor*lenScale)*contactForcePoint->force);

        const ArrowProperties arrowProperties = {
            .start = contactForcePoint->point,
            .end = contactForcePoint->point + (fixupScaleFactor*lenScale)*contactForcePoint->force,
            .tip_length = tip_length,
            .neck_thickness = fixupScaleFactor*baseRadius*0.6f,
            .head_thickness = fixupScaleFactor*baseRadius,
            .color = c_PointForceArrowColor,
            .decoration_flags = SceneDecorationFlag::AnnotationElement,
        };
        draw_arrow(rs.updSceneCache(), arrowProperties, [&hcf, &rs](SceneDecoration&& d)
        {
            rs.consume(hcf, std::move(d));
        });
    }
}

void osc::GenerateModelDecorations(
    SceneCache& meshCache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts,
    float fixupScaleFactor,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out)
{
    GenerateSubcomponentDecorations(
        meshCache,
        model,
        state,
        model,  // i.e. the subcomponent is the root
        opts,
        fixupScaleFactor,
        out,
        false
    );
}

std::vector<SceneDecoration> osc::GenerateModelDecorations(
    SceneCache& cache,
    const IModelStatePair& modelState,
    const OpenSimDecorationOptions& opts,
    float fixupScaleFactor)
{
    return GenerateModelDecorations(
        cache,
        modelState.getModel(),
        modelState.getState(),
        opts,
        fixupScaleFactor
    );
}

std::vector<SceneDecoration> osc::GenerateModelDecorations(
    SceneCache& cache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts,
    float fixupScaleFactor)
{
    std::vector<SceneDecoration> rv;
    ComponentAbsPathDecorationTagger pathTagger;

    GenerateSubcomponentDecorations(
        cache,
        model,
        state,
        model,
        opts,
        fixupScaleFactor,
        [&rv, &pathTagger](const OpenSim::Component& component, SceneDecoration&& decoration)
        {
            pathTagger(component, decoration);
            rv.push_back(std::move(decoration));
        },
        false
    );
    return rv;
}

void osc::GenerateSubcomponentDecorations(
    SceneCache& meshCache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Component& subcomponent,
    const OpenSimDecorationOptions& opts,
    float fixupScaleFactor,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out,
    bool inclusiveOfProvidedSubcomponent)
{
    OSC_PERF("OpenSimRenderer/GenerateModelDecorations");

    RendererState rendererState{
        meshCache,
        model,
        state,
        opts,
        fixupScaleFactor,
        out,
    };

    const auto emitDecorationsForComponent = [&](const OpenSim::Component& c)
    {
        // handle OSC-specific decoration specializations, or fallback to generic
        // component decoration handling
        if (not ShouldShowInUI(c)) {
            return;
        }
        else if (const auto* const custom = dynamic_cast<const ICustomDecorationGenerator*>(&c)) {
            // edge-case: it's a component that has an OSC-specific `ICustomDecorationGenerator`
            //            so we can skip the song-and-dance with caches, OpenSim, SimTK, etc.
            custom->generateCustomDecorations(rendererState.getState(), [&c, &rendererState](SceneDecoration&& dec)
            {
                rendererState.consume(c, std::move(dec));
            });
        }
        else if (const auto* const gp = dynamic_cast<const OpenSim::GeometryPath*>(&c)) {
            HandleGeometryPath(rendererState, *gp);
        }
        else if (const auto* const b = dynamic_cast<const OpenSim::Body*>(&c)) {
            HandleBody(rendererState, *b);
        }
        else if (const auto* const fg = dynamic_cast<const OpenSim::FrameGeometry*>(&c)) {
            HandleFrameGeometry(rendererState, *fg);
        }
        else if (const auto* const p2p = dynamic_cast<const OpenSim::PointToPointSpring*>(&c); p2p and opts.getShouldShowPointToPointSprings()) {
            GenerateBodySpatialVectorArrowDecorationsForForcesThatOnlyHaveComputeForceMethod(rendererState, *p2p);
            HandlePointToPointSpring(rendererState, *p2p);
        }
        else if (typeid(c) == typeid(OpenSim::Station)) {
            // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            HandleStation(rendererState, dynamic_cast<const OpenSim::Station&>(c));
        }
        else if (const auto* const sj = dynamic_cast<const OpenSim::ScapulothoracicJoint*>(&c); sj && opts.getShouldShowScapulo()) {
            HandleScapulothoracicJoint(rendererState, *sj);
        }
        else if (const auto* const hcf = dynamic_cast<const OpenSim::HuntCrossleyForce*>(&c)) {
            GenerateBodySpatialVectorArrowDecorationsForForcesThatOnlyHaveComputeForceMethod(rendererState, *hcf);
            HandleHuntCrossleyForce(rendererState, *hcf);
        }
        else if (dynamic_cast<const OpenSim::Geometry*>(&c)) {
            // EDGE-CASE:
            //
            // if the component being rendered is geometry that was explicitly added into the model then
            // the scene scale factor should not apply to that geometry
            rendererState.emitGenericDecorations(c, c, 1.0f);  // note: override scale factor
        }
        else if (const auto* const forceProducer = dynamic_cast<const OpenSim::ForceProducer*>(&c)) {
            GenerateForceArrowDecorationsFromForceProducer(rendererState, *forceProducer);
            rendererState.emitGenericDecorations(c, c);
        }
        else if (const auto* const force = dynamic_cast<const OpenSim::Force*>(&c)) {
            GenerateBodySpatialVectorArrowDecorationsForForcesThatOnlyHaveComputeForceMethod(rendererState, *force);
            rendererState.emitGenericDecorations(c, c);
        }
        else {
            rendererState.emitGenericDecorations(c, c);
        }
    };

    if (inclusiveOfProvidedSubcomponent) {
        emitDecorationsForComponent(subcomponent);
    }
    for (const OpenSim::Component& c : subcomponent.getComponentList()) {
        emitDecorationsForComponent(c);
    }
}

Mesh osc::ToOscMesh(
    SceneCache& meshCache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh,
    const OpenSimDecorationOptions& opts,
    float fixupScaleFactor)
{
    std::vector<SceneDecoration> decs;
    decs.reserve(1);  // probable
    GenerateSubcomponentDecorations(
        meshCache,
        model,
        state,
        mesh,
        opts,
        fixupScaleFactor,
        [&decs](const OpenSim::Component&, SceneDecoration&& dec)
        {
            decs.push_back(std::move(dec));
        }
    );

    if (decs.empty()) {
        std::stringstream ss;
        ss << mesh.getAbsolutePathString() << ": could not be converted into an OSC mesh because OpenSim did not emit any decorations for the given OpenSim::Mesh component";
        throw std::runtime_error{std::move(ss).str()};
    }
    if (decs.size() > 1) {
        const auto path = mesh.getAbsolutePathString();
        log_warn("%s: this OpenSim::Mesh component generated more than one decoration: OSC defaulted to using the first one, but that may not be correct: if you are seeing unusual behavior, then it's because OpenSim is doing something whacky when generating decorations for a mesh", path.c_str());
    }
    return std::move(decs.front().mesh);
}

Mesh osc::ToOscMesh(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh)
{
    SceneCache cache;
    const OpenSimDecorationOptions opts;
    return ToOscMesh(cache, model, state, mesh, opts, 1.0f);
}

Mesh osc::ToOscMeshBakeScaleFactors(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh)
{
    Mesh rv = ToOscMesh(model, state, mesh);
    rv.transform_vertices({.scale =  to<Vec3>(mesh.get_scale_factors())});

    return rv;
}

float osc::GetRecommendedScaleFactor(
    SceneCache& meshCache,
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSimDecorationOptions& opts)
{
    // generate+union all scene decorations to get an idea of the scene size
    std::optional<AABB> aabb;
    GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        1.0f,
        [&aabb](const OpenSim::Component&, const SceneDecoration& dec)
        {
            aabb = bounding_aabb_of(aabb, world_space_bounds_of(dec));
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
