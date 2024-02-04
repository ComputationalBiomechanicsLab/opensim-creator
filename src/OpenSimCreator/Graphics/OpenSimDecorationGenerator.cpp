#include "OpenSimDecorationGenerator.hpp"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/SimTKDecorationGenerator.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <OpenSimCreator/Utils/SimTKHelpers.hpp>

#include <SimTKcommon.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/Model/PathSpring.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Utils/Perf.hpp>

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    inline constexpr float c_GeometryPathBaseRadius = 0.005f;
    constexpr Color c_EffectiveLineOfActionColor = Color::green();
    constexpr Color c_AnatomicalLineOfActionColor = Color::red();

    // helper: convert a physical frame's transform to ground into an Transform
    Transform TransformInGround(OpenSim::Frame const& frame, SimTK::State const& state)
    {
        return ToTransform(frame.getTransformInGround(state));
    }

    // returns value between [0.0f, 1.0f]
    float GetMuscleColorFactor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
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
            auto const nfl = static_cast<float>(musc.getNormalizedFiberLength(st));  // 1.0f == ideal length
            float fl = nfl - 1.0f;
            fl = std::abs(fl);
            fl = std::min(fl, 1.0f);
            return fl;
        }
        default:
            return 1.0f;
        }
    }

    Color GetGeometryPathDefaultColor(OpenSim::GeometryPath const& gp)
    {
        SimTK::Vec3 const& c = gp.getDefaultColor();
        return Color{ToVec3(c)};
    }

    Color GetGeometryPathColor(OpenSim::GeometryPath const& gp, SimTK::State const& st)
    {
        // returns the same color that OpenSim emits (which is usually just activation-based,
        // but might change in future versions of OpenSim)
        SimTK::Vec3 const c = gp.getColor(st);
        return Color{ToVec3(c)};
    }

    Color CalcOSCMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        MuscleColoringStyle s)
    {
        Color const zeroColor = {50.0f / 255.0f, 50.0f / 255.0f, 166.0f / 255.0f, 1.0f};
        Color const fullColor = {255.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f};
        float const factor = GetMuscleColorFactor(musc, st, s);
        return Lerp(zeroColor, fullColor, factor);
    }

    // helper: returns the color a muscle should have, based on a variety of options (style, user-defined stuff in OpenSim, etc.)
    //
    // this is just a rough estimation of how SCONE is coloring things
    Color GetMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        MuscleColoringStyle s)
    {
        switch (s)
        {
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
    float GetSconeStyleAutomaticMuscleRadiusCalc(OpenSim::Muscle const& m)
    {
        auto const f = static_cast<float>(m.getMaxIsometricForce());
        float const specificTension = 0.25e6f;  // magic number?
        float const pcsa = f / specificTension;
        float const widthFactor = 0.25f;
        return widthFactor * std::sqrt(pcsa / std::numbers::pi_v<float>);
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(
        OpenSim::Muscle const& musc,
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
            OpenSim::Model const& model,
            SimTK::State const& state,
            OpenSimDecorationOptions const& opts,
            float fixupScaleFactor,
            std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out) :

            m_MeshCache{meshCache},
            m_Model{model},
            m_State{state},
            m_Opts{opts},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Out{out}
        {
        }

        SceneCache& updMeshCache()
        {
            return m_MeshCache;
        }

        Mesh const& getSphereMesh() const
        {
            return m_SphereMesh;
        }

        Mesh const& getCylinderMesh() const
        {
            return m_CylinderMesh;
        }

        OpenSim::ModelDisplayHints const& getModelDisplayHints() const
        {
            return m_ModelDisplayHints;
        }

        bool getShowPathPoints() const
        {
            return m_ShowPathPoints;
        }

        SimTK::SimbodyMatterSubsystem const& getMatterSubsystem() const
        {
            return m_MatterSubsystem;
        }

        SimTK::State const& getState() const
        {
            return m_State;
        }

        OpenSimDecorationOptions const& getOptions() const
        {
            return m_Opts;
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model;
        }

        float getFixupScaleFactor() const
        {
            return m_FixupScaleFactor;
        }

        void consume(OpenSim::Component const& component, SceneDecoration&& dec)
        {
            m_Out(component, std::move(dec));
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emitGenericDecorations(
            OpenSim::Component const& componentToRender,
            OpenSim::Component const& componentToLinkTo,
            float fixupScaleFactor)
        {
            std::function<void(SceneDecoration&&)> const callback = [this, &componentToLinkTo](SceneDecoration&& dec)
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
            for (SimTK::DecorativeGeometry const& geom : m_GeomList)
            {
                GenerateDecorations(
                    updMeshCache(),
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
            for (SimTK::DecorativeGeometry const& geom : m_GeomList)
            {
                GenerateDecorations(
                    updMeshCache(),
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
            OpenSim::Component const& componentToRender,
            OpenSim::Component const& componentToLinkTo)
        {
            emitGenericDecorations(componentToRender, componentToLinkTo, getFixupScaleFactor());
        }

    private:
        SceneCache& m_MeshCache;
        Mesh m_SphereMesh = m_MeshCache.getSphereMesh();
        Mesh m_CylinderMesh = m_MeshCache.getCylinderMesh();
        OpenSim::Model const& m_Model;
        OpenSim::ModelDisplayHints const& m_ModelDisplayHints = m_Model.getDisplayHints();
        bool m_ShowPathPoints = m_ModelDisplayHints.get_show_path_points();
        SimTK::SimbodyMatterSubsystem const& m_MatterSubsystem = m_Model.getSystem().getMatterSubsystem();
        SimTK::State const& m_State;
        OpenSimDecorationOptions const& m_Opts;
        float m_FixupScaleFactor;
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& m_Out;
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomList;
    };

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void HandlePointToPointSpring(
        RendererState& rs,
        OpenSim::PointToPointSpring const& p2p)
    {
        if (!rs.getOptions().getShouldShowPointToPointSprings())
        {
            return;
        }

        Vec3 const p1 = TransformInGround(p2p.getBody1(), rs.getState()) * ToVec3(p2p.getPoint1());
        Vec3 const p2 = TransformInGround(p2p.getBody2(), rs.getState()) * ToVec3(p2p.getPoint2());

        float const radius = c_GeometryPathBaseRadius * rs.getFixupScaleFactor();
        Transform const cylinderXform = YToYCylinderToSegmentTransform({p1, p2}, radius);

        rs.consume(p2p, SceneDecoration
        {
            .mesh = rs.getCylinderMesh(),
            .transform = cylinderXform,
            .color = {0.7f, 0.7f, 0.7f, 1.0f},
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(
        RendererState& rs,
        OpenSim::Station const& s)
    {
        float radius = rs.getFixupScaleFactor() * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        Transform xform;
        xform.position = ToVec3(s.getLocationInGround(rs.getState()));
        xform.scale = {radius, radius, radius};

        rs.consume(s, SceneDecoration
        {
            .mesh = rs.getSphereMesh(),
            .transform = xform,
            .color = Color::red(),
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void HandleScapulothoracicJoint(
        RendererState& rs,
        OpenSim::ScapulothoracicJoint const& scapuloJoint)
    {
        Transform t = TransformInGround(scapuloJoint.getParentFrame(), rs.getState());
        t.scale = ToVec3(scapuloJoint.get_thoracic_ellipsoid_radii_x_y_z());

        rs.consume(scapuloJoint, SceneDecoration
        {
            .mesh = rs.getSphereMesh(),
            .transform = t,
            .color = {1.0f, 1.0f, 0.0f, 0.2f},
        });
    }

    // OSC-specific decoration handler for body centers of mass
    void HandleBodyCentersOfMass(
        RendererState& rs,
        OpenSim::Body const& b)
    {
        if (rs.getOptions().getShouldShowCentersOfMass() && b.getMassCenter() != SimTK::Vec3{0.0, 0.0, 0.0})
        {
            float const radius = rs.getFixupScaleFactor() * 0.005f;
            Transform t = TransformInGround(b, rs.getState());
            t.position = TransformPoint(t, ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            rs.consume(b, SceneDecoration
            {
                .mesh = rs.getSphereMesh(),
                .transform = t,
                .color = Color::black(),
            });
        }
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(
        RendererState& rs,
        OpenSim::Body const& b)
    {
        HandleBodyCentersOfMass(rs, b);  // CoMs are handled by OSC
        rs.emitGenericDecorations(b, b);  // bodies are emitted by OpenSim
    }

    // OSC-specific decoration handler for Muscle+Fiber representation of an `OpenSim::Muscle`
    void HandleMuscleFibersAndTendons(
        RendererState& rs,
        OpenSim::Muscle const& muscle)
    {
        std::vector<GeometryPathPoint> const pps = GetAllPathPoints(muscle.getGeometryPath(), rs.getState());
        if (pps.empty())
        {
            return;  // edge-case: there are no points in the muscle path
        }

        float const fixupScaleFactor = rs.getFixupScaleFactor();

        float const fiberUiRadius = GetMuscleSize(
            muscle,
            fixupScaleFactor,
            rs.getOptions().getMuscleSizingStyle()
        );
        float const tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        Color const fiberColor = GetMuscleColor(
            muscle,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );
        Color const tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f};

        SceneDecoration const tendonSpherePrototype =
        {
            .mesh = rs.getSphereMesh(),
            .transform = {.scale = Vec3{tendonUiRadius}},
            .color = tendonColor,
        };
        SceneDecoration const tendonCylinderPrototype =
        {
            .mesh = rs.getCylinderMesh(),
            .color = tendonColor,
        };
        SceneDecoration const fiberSpherePrototype =
        {
            .mesh = rs.getSphereMesh(),
            .transform = {.scale = Vec3{fiberUiRadius}},
            .color = fiberColor,
        };
        SceneDecoration const fiberCylinderPrototype =
        {
            .mesh = rs.getCylinderMesh(),
            .color = fiberColor,
        };

        auto const emitTendonSphere = [&](GeometryPathPoint const& p)
        {
            OpenSim::Component const* c = &muscle;
            if (p.maybeUnderlyingUserPathPoint)
            {
                c = p.maybeUnderlyingUserPathPoint;
            }
            rs.consume(*c, tendonSpherePrototype.withPosition(p.locationInGround));
        };
        auto const emitTendonCylinder = [&](Vec3 const& p1, Vec3 const& p2)
        {
            Transform const xform = YToYCylinderToSegmentTransform({p1, p2}, tendonUiRadius);
            rs.consume(muscle, tendonCylinderPrototype.withTransform(xform));
        };
        auto emitFiberSphere = [&](GeometryPathPoint const& p)
        {
            OpenSim::Component const* c = &muscle;
            if (p.maybeUnderlyingUserPathPoint)
            {
                c = p.maybeUnderlyingUserPathPoint;
            }
            rs.consume(*c, fiberSpherePrototype.withPosition(p.locationInGround));
        };
        auto emitFiberCylinder = [&](Vec3 const& p1, Vec3 const& p2)
        {
            Transform const xform = YToYCylinderToSegmentTransform({p1, p2}, fiberUiRadius);
            rs.consume(muscle, fiberCylinderPrototype.withTransform(xform));
        };

        if (pps.size() == 1)
        {
            emitFiberSphere(pps.front());  // paranoia: shouldn't happen
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it
        float const tendonLen = std::max(0.0f, static_cast<float>(muscle.getTendonLength(rs.getState()) * 0.5));
        float const fiberLen = std::max(0.0f, static_cast<float>(muscle.getFiberLength(rs.getState())));
        float const fiberEnd = tendonLen + fiberLen;
        bool const hasTendonSpheres = tendonLen > 0.0f;

        size_t i = 1;
        GeometryPathPoint prevPoint = pps.front();
        float prevTraversalPos = 0.0f;

        // draw first tendon
        if (prevTraversalPos < tendonLen)
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint);
        }
        while (i < pps.size() && prevTraversalPos < tendonLen)
        {
            // emit remaining tendon cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            Vec3 const prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = Length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f)
            {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                Vec3 tendonEnd = prevPoint.locationInGround + scaler * prevToPos;

                emitTendonCylinder(prevPoint.locationInGround, tendonEnd);
                emitTendonSphere(GeometryPathPoint{tendonEnd});

                prevPoint.locationInGround = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else
            {
                emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
                emitTendonSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit first fiber sphere (label it if no tendon spheres were emitted)
            emitFiberSphere(hasTendonSpheres ? GeometryPathPoint{prevPoint.locationInGround} : prevPoint);
        }
        while (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit remaining fiber cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = Length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f)
            {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                Vec3 fiberEndPos = prevPoint.locationInGround + scaler * prevToPos;

                emitFiberCylinder(prevPoint.locationInGround, fiberEndPos);
                emitFiberSphere(GeometryPathPoint{fiberEndPos});

                prevPoint.locationInGround = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else
            {
                emitFiberCylinder(prevPoint.locationInGround, point.locationInGround);
                emitFiberSphere(point);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw second tendon
        if (i < pps.size())
        {
            // emit first tendon sphere
            emitTendonSphere(GeometryPathPoint{prevPoint});
        }
        while (i < pps.size())
        {
            // emit remaining fiber cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            Vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = Length(prevToPos);
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
        OpenSim::Component const& hittestTarget,
        std::span<GeometryPathPoint const> points,
        float radius,
        Color const& color)
    {
        if (points.empty())
        {
            // edge-case: there's no points to emit
            return;
        }

        // helper function: emits a sphere decoration
        auto const emitSphere = [&rs, &hittestTarget, radius, color](
            GeometryPathPoint const& pp,
            Vec3 const& upDirection)
        {
            // ensure that user-defined path points are independently selectable (#425)
            OpenSim::Component const& c = pp.maybeUnderlyingUserPathPoint ?
                *pp.maybeUnderlyingUserPathPoint :
                hittestTarget;

            rs.consume(c, SceneDecoration
            {
                .mesh = rs.getSphereMesh(),
                .transform =
                {
                    // ensure the sphere directionally tries to line up with the cylinders, to make
                    // the "join" between the sphere and cylinders nicer (#593)
                    .scale = Vec3{radius},
                    .rotation = Normalize(Rotation(Vec3{0.0f, 1.0f, 0.0f}, upDirection)),
                    .position = pp.locationInGround
                },
                .color = color,
            });
        };

        // helper function: emits a cylinder decoration between two points
        auto const emitCylinder = [&rs, &hittestTarget, radius, color](
            Vec3 const& p1,
            Vec3 const& p2)
        {
            rs.consume(hittestTarget, SceneDecoration
            {
                .mesh = rs.getCylinderMesh(),
                .transform = YToYCylinderToSegmentTransform({p1, p2}, radius),
                .color  = color,
            });
        };

        // if required, draw first path point
        if (rs.getShowPathPoints())
        {
            GeometryPathPoint const& firstPoint = points.front();
            Vec3 const& ppPos = firstPoint.locationInGround;
            Vec3 const direction = points.size() == 1 ?
                Vec3{0.0f, 1.0f, 0.0f} :
                Normalize(points[1].locationInGround - ppPos);

            emitSphere(firstPoint, direction);
        }

        // draw remaining cylinders and (if required) muscle path points
        for (size_t i = 1; i < points.size(); ++i)
        {
            GeometryPathPoint const& point = points[i];

            Vec3 const& prevPos = points[i - 1].locationInGround;
            Vec3 const& curPos = point.locationInGround;

            emitCylinder(prevPos, curPos);

            // if required, draw path points
            if (rs.getShowPathPoints())
            {
                Vec3 const direction = Normalize(curPos - prevPos);
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
        OpenSim::Muscle const& musc)
    {
        std::vector<GeometryPathPoint> const points =
            GetAllPathPoints(musc.getGeometryPath(), rs.getState());

        float const radius = GetMuscleSize(
            musc,
            rs.getFixupScaleFactor(),
            rs.getOptions().getMuscleSizingStyle()
        );

        Color const color = GetMuscleColor(
            musc,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );

        EmitPointBasedLine(rs, musc, points, radius, color);
    }

    // custom implementation of `OpenSim::GeometryPath::generateDecorations` that also
    // handles tagging
    void HandleGenericGeometryPath(
        RendererState& rs,
        OpenSim::GeometryPath const& gp,
        OpenSim::Component const& hittestTarget)
    {
        // this specialized `OpenSim::GeometryPath` handler is used, rather than
        // `emitGenericDecorations`, because the custom implementation also coerces
        // selection hits to enable users to click on individual path points within
        // a path (#647)

        std::vector<GeometryPathPoint> const points = GetAllPathPoints(gp, rs.getState());
        Color const color = GetGeometryPathColor(gp, rs.getState());

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
        OpenSim::Muscle const& muscle,
        PointDirection const& loaPointDirection,
        Color const& color)
    {
        float const fixupScaleFactor = rs.getFixupScaleFactor();

        ArrowProperties p;
        p.worldspaceStart = loaPointDirection.point;
        p.worldspaceEnd = loaPointDirection.point + (fixupScaleFactor*0.1f)*loaPointDirection.direction;
        p.tipLength = (fixupScaleFactor*0.015f);
        p.headThickness = (fixupScaleFactor*0.01f);
        p.neckThickness = (fixupScaleFactor*0.006f);
        p.color = color;

        DrawArrow(rs.updMeshCache(), p, [&muscle, &rs](SceneDecoration&& d)
        {
            rs.consume(muscle, std::move(d));
        });
    }

    void HandleLinesOfAction(
        RendererState& rs,
        OpenSim::Muscle const& musc)
    {
        // if options request, render effective muscle lines of action
        if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin() ||
            rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion())
        {
            if (std::optional<LinesOfAction> const loas = GetEffectiveLinesOfActionInGround(musc, rs.getState()))
            {
                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin())
                {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_EffectiveLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion())
                {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_EffectiveLineOfActionColor);
                }
            }
        }

        // if options request, render anatomical muscle lines of action
        if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin() ||
            rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion())
        {
            if (std::optional<LinesOfAction> const loas = GetAnatomicalLinesOfActionInGround(musc, rs.getState()))
            {
                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin())
                {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_AnatomicalLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion())
                {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_AnatomicalLineOfActionColor);
                }
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(
        RendererState& rs,
        OpenSim::GeometryPath const& gp)
    {
        if (!gp.get_Appearance().get_visible())
        {
            // even custom muscle decoration implementations *must* obey the visibility
            // flag on `GeometryPath` (#414)
            return;
        }

        if (!gp.hasOwner())
        {
            // it's a standalone path that's not part of a muscle
            HandleGenericGeometryPath(rs, gp, gp);
            return;
        }

        // the `GeometryPath` has a owner, which might be a muscle or path actuator
        if (auto const* const musc = GetOwner<OpenSim::Muscle>(gp))
        {
            // owner is a muscle, coerce selection "hit" to the muscle

            HandleLinesOfAction(rs, *musc);

            switch (rs.getOptions().getMuscleDecorationStyle())
            {
            case MuscleDecorationStyle::FibersAndTendons:
                HandleMuscleFibersAndTendons(rs, *musc);
                return;
            case MuscleDecorationStyle::Hidden:
                return;  // just don't generate them
            case MuscleDecorationStyle::OpenSim:
            default:
                HandleMuscleOpenSimStyle(rs, *musc);
                return;
            }
        }
        else if (auto const* const pa = GetOwner<OpenSim::PathActuator>(gp))
        {
            // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
            HandleGenericGeometryPath(rs, gp, *pa);
            return;
        }
        else if (auto const* const pathSpring = GetOwner<OpenSim::PathSpring>(gp))
        {
            // owner is a path spring, coerce selection "hit" to the path spring (#650)
            HandleGenericGeometryPath(rs, gp, *pathSpring);
            return;
        }
        else
        {
            // it's a path in some non-muscular context
            HandleGenericGeometryPath(rs, gp, gp);
            return;
        }
    }

    void HandleFrameGeometry(
        RendererState& rs,
        OpenSim::FrameGeometry const& frameGeometry)
    {
        // promote current component to the parent of the frame geometry, because
        // a user is probably more interested in the thing the frame geometry
        // represents (e.g. an offset frame) than the geometry itself (#506)
        OpenSim::Component const& componentToLinkTo =
            GetOwnerOr(frameGeometry, frameGeometry);

        rs.emitGenericDecorations(frameGeometry, componentToLinkTo);
    }

    void HandleHuntCrossleyForce(
        RendererState& rs,
        OpenSim::HuntCrossleyForce const& hcf)
    {
        if (!rs.getOptions().getShouldShowContactForces())
        {
            return;  // the user hasn't opted to see contact forces
        }

        // IGNORE: rs.getModelDisplayHints().get_show_forces()
        //
        // because this is a user-enacted UI option and it would be silly
        // to expect the user to *also* toggle the "show_forces" option inside
        // the OpenSim model

        if (!hcf.appliesForce(rs.getState()))
        {
            return;  // not applying this force
        }

        // else: try and compute a geometry-to-plane contact force and show it in-UI
        std::optional<ForcePoint> const maybeContact = TryGetContactForceInGround(
            rs.getModel(),
            rs.getState(),
            hcf
        );

        if (!maybeContact)
        {
            return;
        }

        float const fixupScaleFactor = rs.getFixupScaleFactor();
        float const lenScale = 0.0025f;
        float const baseRadius = 0.025f;
        float const tipLength = 0.1f*Length((fixupScaleFactor*lenScale)*maybeContact->force);

        ArrowProperties p;
        p.worldspaceStart = maybeContact->point;
        p.worldspaceEnd = maybeContact->point + (fixupScaleFactor*lenScale)*maybeContact->force;
        p.tipLength = tipLength;
        p.headThickness = fixupScaleFactor*baseRadius;
        p.neckThickness = fixupScaleFactor*baseRadius*0.6f;
        p.color = Color::yellow();

        DrawArrow(rs.updMeshCache(), p, [&hcf, &rs](SceneDecoration&& d)
        {
            rs.consume(hcf, std::move(d));
        });
    }
}

void osc::GenerateModelDecorations(
    SceneCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSimDecorationOptions const& opts,
    float fixupScaleFactor,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
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
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Component const& subcomponent,
    OpenSimDecorationOptions const& opts,
    float fixupScaleFactor,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out,
    bool inclusiveOfProvidedSubcomponent)
{
    OSC_PERF("OpenSimRenderer/GenerateModelDecorations");

    RendererState rendererState
    {
        meshCache,
        model,
        state,
        opts,
        fixupScaleFactor,
        out,
    };

    auto const emitDecorationsForComponent = [&](OpenSim::Component const& c)
    {
        // handle OSC-specific decoration specializations, or fallback to generic
        // component decoration handling
        if (!ShouldShowInUI(c))
        {
            return;
        }
        else if (auto const* const gp = dynamic_cast<OpenSim::GeometryPath const*>(&c))
        {
            HandleGeometryPath(rendererState, *gp);
        }
        else if (auto const* const b = dynamic_cast<OpenSim::Body const*>(&c))
        {
            HandleBody(rendererState, *b);
        }
        else if (auto const* const fg = dynamic_cast<OpenSim::FrameGeometry const*>(&c))
        {
            HandleFrameGeometry(rendererState, *fg);
        }
        else if (auto const* const p2p = dynamic_cast<OpenSim::PointToPointSpring const*>(&c); p2p && opts.getShouldShowPointToPointSprings())
        {
            HandlePointToPointSpring(rendererState, *p2p);
        }
        else if (typeid(c) == typeid(OpenSim::Station))
        {
            // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            HandleStation(rendererState, dynamic_cast<OpenSim::Station const&>(c));
        }
        else if (auto const* const sj = dynamic_cast<OpenSim::ScapulothoracicJoint const*>(&c); sj && opts.getShouldShowScapulo())
        {
            HandleScapulothoracicJoint(rendererState, *sj);
        }
        else if (auto const* const hcf = dynamic_cast<OpenSim::HuntCrossleyForce const*>(&c))
        {
            HandleHuntCrossleyForce(rendererState, *hcf);
        }
        else if (auto const* const geom = dynamic_cast<OpenSim::Geometry const*>(&c))
        {
            // EDGE-CASE:
            //
            // if the component being rendered is geometry that was explicitly added into the model then
            // the scene scale factor should not apply to that geometry
            rendererState.emitGenericDecorations(c, c, 1.0f);  // note: override scale factor
        }
        else
        {
            rendererState.emitGenericDecorations(c, c);
        }
    };

    if (inclusiveOfProvidedSubcomponent)
    {
        emitDecorationsForComponent(subcomponent);
    }
    for (OpenSim::Component const& c : subcomponent.getComponentList())
    {
        emitDecorationsForComponent(c);
    }
}

Mesh osc::ToOscMesh(
    SceneCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Mesh const& mesh,
    OpenSimDecorationOptions const& opts,
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
        [&decs](OpenSim::Component const&, SceneDecoration&& dec)
        {
            decs.push_back(std::move(dec));
        }
    );

    if (decs.empty())
    {
        std::stringstream ss;
        ss << mesh.getAbsolutePathString() << ": could not be converted into an OSC mesh because OpenSim did not emit any decorations for the given OpenSim::Mesh component";
        throw std::runtime_error{std::move(ss).str()};
    }
    if (decs.size() > 1)
    {
        auto path = mesh.getAbsolutePathString();
        log_warn("%s: this OpenSim::Mesh component generated more than one decoration: OSC defaulted to using the first one, but that may not be correct: if you are seeing unusual behavior, then it's because OpenSim is doing something whacky when generating decorations for a mesh", path.c_str());
    }
    return std::move(decs[0].mesh);
}

Mesh osc::ToOscMesh(
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Mesh const& mesh)
{
    SceneCache cache;
    OpenSimDecorationOptions opts;
    return ToOscMesh(cache, model, state, mesh, opts, 1.0f);
}

Mesh osc::ToOscMeshBakeScaleFactors(
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Mesh const& mesh)
{
    Mesh rv = ToOscMesh(model, state, mesh);

    Transform t;
    t.scale = ToVec3(mesh.get_scale_factors());
    rv.transformVerts(t);

    return rv;
}

float osc::GetRecommendedScaleFactor(
    SceneCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSimDecorationOptions const& opts)
{
    // generate+union all scene decorations to get an idea of the scene size
    std::optional<AABB> aabb;
    GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        1.0f,
        [&aabb](OpenSim::Component const&, SceneDecoration&& dec)
        {
            AABB const decorationAABB = GetWorldspaceAABB(dec);
            aabb = aabb ? Union(*aabb, decorationAABB) : decorationAABB;
        }
    );

    if (!aabb)
    {
        return 1.0f;  // no scene elements
    }

    // calculate the longest dimension and use that to figure out
    // what the smallest scale factor that would cause that dimension
    // to be >=1 cm (roughly the length of a frame leg in OSC's
    // decoration generator)
    float longest = LongestDim(*aabb);
    float rv = 1.0f;
    while (longest < 0.01)
    {
        longest *= 10.0f;
        rv /= 10.0f;
    }

    return rv;
}
