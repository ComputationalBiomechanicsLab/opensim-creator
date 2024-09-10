#include "OpenSimDecorationGenerator.h"

#include <OpenSimCreator/Documents/CustomComponents/ICustomDecorationGenerator.h>
#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

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
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Perf.h>
#include <oscar_simbody/SimTKDecorationGenerator.h>
#include <oscar_simbody/SimTKHelpers.h>
#include <SimTKcommon.h>

#include <ankerl/unordered_dense.h>

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    inline constexpr float c_GeometryPathBaseRadius = 0.005f;
    inline constexpr float c_ForceArrowLengthScale = 0.005f;
    inline constexpr float c_TorqueArrowLengthScale = 0.01f;
    inline constexpr Color c_EffectiveLineOfActionColor = Color::green();
    inline constexpr Color c_AnatomicalLineOfActionColor = Color::red();
    inline constexpr Color c_BodyForceArrowColor = Color::yellow();
    inline constexpr Color c_BodyTorqueArrowColor = Color::orange();
    inline constexpr Color c_PointForceArrowColor = Color::muted_yellow();  // note: should be similar to body force arrows
    inline constexpr Color c_StationColor = Color::red();
    inline constexpr Color c_ScapulothoracicJointColor =  Color::yellow().with_alpha(0.2f);
    inline constexpr Color c_CenterOfMassColor = Color::black();

    // helper: convert a physical frame's transform to ground into an Transform
    Transform TransformInGround(const OpenSim::Frame& frame, const SimTK::State& state)
    {
        return decompose_to_transform(frame.getTransformInGround(state));
    }

    // returns value between [0.0f, 1.0f]
    float GetMuscleColorFactor(
        const OpenSim::Muscle& musc,
        const SimTK::State& st,
        MuscleColoringStyle s)
    {
        switch (s) {
        case MuscleColoringStyle::Activation:
            return static_cast<float>(musc.getActivation(st));
        case MuscleColoringStyle::Excitation:
            return static_cast<float>(musc.getExcitation(st));
        case MuscleColoringStyle::Force:
            return static_cast<float>(musc.getActuation(st)) / static_cast<float>(musc.getMaxIsometricForce());
        case MuscleColoringStyle::FiberLength:
        {
            const auto nfl = static_cast<float>(musc.getNormalizedFiberLength(st));  // 1.0f == ideal length
            float fl = nfl - 1.0f;
            fl = abs(fl);
            fl = min(fl, 1.0f);
            return fl;
        }
        default:
            return 1.0f;
        }
    }

    Color GetGeometryPathDefaultColor(const OpenSim::GeometryPath& gp)
    {
        const SimTK::Vec3& c = gp.getDefaultColor();
        return Color{ToVec3(c)};
    }

    Color GetGeometryPathColor(const OpenSim::GeometryPath& gp, const SimTK::State& st)
    {
        // returns the same color that OpenSim emits (which is usually just activation-based,
        // but might change in future versions of OpenSim)
        const SimTK::Vec3 c = gp.getColor(st);
        return Color{ToVec3(c)};
    }

    Color CalcOSCMuscleColor(
        const OpenSim::Muscle& musc,
        const SimTK::State& st,
        MuscleColoringStyle s)
    {
        const Color zeroColor = {50.0f / 255.0f, 50.0f / 255.0f, 166.0f / 255.0f, 1.0f};
        const Color fullColor = {255.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f};
        const float factor = GetMuscleColorFactor(musc, st, s);
        return lerp(zeroColor, fullColor, factor);
    }

    // helper: returns the color a muscle should have, based on a variety of options (style, user-defined stuff in OpenSim, etc.)
    //
    // this is just a rough estimation of how SCONE is coloring things
    Color GetMuscleColor(
        const OpenSim::Muscle& musc,
        const SimTK::State& st,
        MuscleColoringStyle s)
    {
        switch (s) {
        case MuscleColoringStyle::OpenSimAppearanceProperty:
            return GetGeometryPathDefaultColor(musc.getGeometryPath());
        case MuscleColoringStyle::OpenSim:
            return GetGeometryPathColor(musc.getGeometryPath(), st);
        default:
            return CalcOSCMuscleColor(musc, st, s);
        }
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
        case MuscleSizingStyle::OpenSim:
        default:
            return c_GeometryPathBaseRadius * fixupScaleFactor;
        }
    }
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

    private:
        SceneCache& m_MeshCache;
        Mesh m_SphereMesh = m_MeshCache.sphere_mesh();
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
                    .start = ToVec3(positionInGround),
                    .end = ToVec3(positionInGround + (fixupScaleFactor * c_ForceArrowLengthScale * forceInGround)),
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
                .start = ToVec3(frame2ground * SimTK::Vec3{0.0}),
                .end = ToVec3(frame2ground * (fixupScaleFactor * c_TorqueArrowLengthScale * torqueInGround)),
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
                .start = ToVec3(frame2ground * SimTK::Vec3{0.0}),
                .end = ToVec3(frame2ground * (fixupScaleFactor * c_ForceArrowLengthScale * forceInGround)),
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
        ankerl::unordered_dense::map<const OpenSim::PhysicalFrame*, SimTK::SpatialVec> m_AccumulatedBodySpatialVecs;
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
                    .start = ToVec3(mobod2ground * SimTK::Vec3{0.0}),
                    .end = ToVec3(mobod2ground * (fixupScaleFactor * c_ForceArrowLengthScale * forceVec)),
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
                    .start = ToVec3(mobod2ground * SimTK::Vec3{0.0}),
                    .end = ToVec3(mobod2ground * (fixupScaleFactor * c_TorqueArrowLengthScale * torqueVec)),
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
            not rs.getOptions().getShouldShowPointTorques() and
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

        const Vec3 p1 = TransformInGround(p2p.getBody1(), rs.getState()) * ToVec3(p2p.getPoint1());
        const Vec3 p2 = TransformInGround(p2p.getBody2(), rs.getState()) * ToVec3(p2p.getPoint2());

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
                .position = ToVec3(s.getLocationInGround(rs.getState())),
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
        t.scale = ToVec3(scapuloJoint.get_thoracic_ellipsoid_radii_x_y_z());

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

        const float radius = rs.getFixupScaleFactor() * 0.005f;
        Transform t = TransformInGround(b, rs.getState());
        t.position = t * ToVec3(b.getMassCenter());
        t.scale = Vec3{radius};

        rs.consume(b, SceneDecoration{
            .mesh = rs.sphere_mesh(),
            .transform = t,
            .shading = c_CenterOfMassColor,
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

        const Color fiberColor = GetMuscleColor(
            muscle,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );
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
            rs.consume(*c, tendonSpherePrototype.with_position(p.locationInGround));
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
            rs.consume(*c, fiberSpherePrototype.with_position(p.locationInGround));
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
        float prevTraversalPos = 0.0f;

        // emit first sphere for first tendon
        if (prevTraversalPos < tendonLen) {
            emitTendonSphere(prevPoint);  // emit first tendon sphere
        }

        // emit remaining cylinders + spheres for first tendon
        while (i < pps.size() && prevTraversalPos < tendonLen) {

            const GeometryPathPoint& point = pps[i];
            const Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f) {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                Vec3 tendonEnd = prevPoint.locationInGround + scaler * prevToPos;

                emitTendonCylinder(prevPoint.locationInGround, tendonEnd);
                emitTendonSphere(GeometryPathPoint{tendonEnd});

                prevPoint.locationInGround = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else {
                emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
                emitTendonSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // emit first sphere for fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd) {
            // label the sphere if no tendon spheres were previously emitted
            emitFiberSphere(hasTendonSpheres ? GeometryPathPoint{prevPoint.locationInGround} : prevPoint);
        }

        // emit remaining cylinders + spheres for fiber
        while (i < pps.size() && prevTraversalPos < fiberEnd) {

            const GeometryPathPoint& point = pps[i];
            Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f) {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                Vec3 fiberEndPos = prevPoint.locationInGround + scaler * prevToPos;

                emitFiberCylinder(prevPoint.locationInGround, fiberEndPos);
                emitFiberSphere(GeometryPathPoint{fiberEndPos});

                prevPoint.locationInGround = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else {
                emitFiberCylinder(prevPoint.locationInGround, point.locationInGround);
                emitFiberSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // emit first sphere for second tendon
        if (i < pps.size()) {
            emitTendonSphere(GeometryPathPoint{prevPoint});
        }

        // emit remaining cylinders + spheres for second tendon
        while (i < pps.size()) {

            const GeometryPathPoint& point = pps[i];
            Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;

            emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
            emitTendonSphere(point);

            i++;
            prevPoint = point;
            prevTraversalPos = traversalPos;
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
                    .position = pp.locationInGround
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
    void HandleMuscleOpenSimStyle(
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

        const Color color = GetMuscleColor(
            musc,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );

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
            case MuscleDecorationStyle::OpenSim:
            default:
                HandleMuscleOpenSimStyle(rs, *muscle);
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
        else if (const auto* const geom = dynamic_cast<const OpenSim::Geometry*>(&c)) {
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
    OpenSimDecorationOptions opts;
    return ToOscMesh(cache, model, state, mesh, opts, 1.0f);
}

Mesh osc::ToOscMeshBakeScaleFactors(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::Mesh& mesh)
{
    Mesh rv = ToOscMesh(model, state, mesh);
    rv.transform_vertices({.scale =  ToVec3(mesh.get_scale_factors())});

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
        [&aabb](const OpenSim::Component&, SceneDecoration&& dec)
        {
            aabb = bounding_aabb_of(aabb, worldspace_bounds_of(dec));
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
