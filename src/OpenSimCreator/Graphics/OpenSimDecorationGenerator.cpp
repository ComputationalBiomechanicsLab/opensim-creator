#include "OpenSimDecorationGenerator.hpp"

#include "OpenSimCreator/Bindings/SimTKDecorationGenerator.hpp"
#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Constants.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/Perf.hpp>

#include <glm/vec3.hpp>
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
#include <SimTKcommon.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    inline constexpr float c_GeometryPathBaseRadius = 0.005f;
    constexpr osc::Color c_EffectiveLineOfActionColor = osc::Color::green();
    constexpr osc::Color c_AnatomicalLineOfActionColor = osc::Color::red();

    // helper: convert a physical frame's transform to ground into an osc::Transform
    osc::Transform TransformInGround(OpenSim::Frame const& frame, SimTK::State const& state)
    {
        return osc::ToTransform(frame.getTransformInGround(state));
    }

    // returns value between [0.0f, 1.0f]
    float GetMuscleColorFactor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
    {
        switch (s) {
        case osc::MuscleColoringStyle::Activation:
            return static_cast<float>(musc.getActivation(st));
        case osc::MuscleColoringStyle::Excitation:
            return static_cast<float>(musc.getExcitation(st));
        case osc::MuscleColoringStyle::Force:
            return static_cast<float>(musc.getActuation(st)) / static_cast<float>(musc.getMaxIsometricForce());
        case osc::MuscleColoringStyle::FiberLength:
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

    osc::Color GetGeometryPathDefaultColor(OpenSim::GeometryPath const& gp)
    {
        SimTK::Vec3 const& c = gp.getDefaultColor();
        return osc::Color{osc::ToVec3(c)};
    }

    osc::Color GetGeometryPathColor(OpenSim::GeometryPath const& gp, SimTK::State const& st)
    {
        // returns the same color that OpenSim emits (which is usually just activation-based,
        // but might change in future versions of OpenSim)
        SimTK::Vec3 const c = gp.getColor(st);
        return osc::Color{osc::ToVec3(c)};
    }

    osc::Color CalcOSCMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
    {
        osc::Color const zeroColor = {50.0f / 255.0f, 50.0f / 255.0f, 166.0f / 255.0f, 1.0f};
        osc::Color const fullColor = {255.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f};
        float const factor = GetMuscleColorFactor(musc, st, s);
        return osc::Lerp(zeroColor, fullColor, factor);
    }

    // helper: returns the color a muscle should have, based on a variety of options (style, user-defined stuff in OpenSim, etc.)
    //
    // this is just a rough estimation of how SCONE is coloring things
    osc::Color GetMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
    {
        switch (s)
        {
        case osc::MuscleColoringStyle::OpenSimAppearanceProperty:
            return GetGeometryPathDefaultColor(musc.getGeometryPath());
        case osc::MuscleColoringStyle::OpenSim:
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
        return widthFactor * std::sqrt(pcsa / osc::fpi);
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(
        OpenSim::Muscle const& musc,
        float fixupScaleFactor,
        osc::MuscleSizingStyle s)
    {
        switch (s) {
        case osc::MuscleSizingStyle::PcsaDerived:
            return GetSconeStyleAutomaticMuscleRadiusCalc(musc) * fixupScaleFactor;
        case osc::MuscleSizingStyle::OpenSim:
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
            osc::MeshCache& meshCache,
            OpenSim::Model const& model,
            SimTK::State const& state,
            osc::OpenSimDecorationOptions const& opts,
            float fixupScaleFactor,
            std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out) :

            m_MeshCache{meshCache},
            m_Model{model},
            m_State{state},
            m_Opts{opts},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Out{out}
        {
        }

        osc::MeshCache& updMeshCache()
        {
            return m_MeshCache;
        }

        osc::Mesh const& getSphereMesh() const
        {
            return m_SphereMesh;
        }

        osc::Mesh const& getCylinderMesh() const
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

        osc::OpenSimDecorationOptions const& getOptions() const
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

        void consume(OpenSim::Component const& component, osc::SceneDecoration&& dec)
        {
            m_Out(component, std::move(dec));
        }

        // use OpenSim to emit generic decorations exactly as OpenSim would emit them
        void emitGenericDecorations(
            OpenSim::Component const& componentToRender,
            OpenSim::Component const& componentToLinkTo,
            float fixupScaleFactor)
        {
            std::function<void(osc::SimpleSceneDecoration&&)> const callback = [this, &componentToLinkTo](osc::SimpleSceneDecoration&& dec)
            {
                consume(componentToLinkTo, osc::SceneDecoration{std::move(dec)});
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
                osc::GenerateDecorations(
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
                osc::GenerateDecorations(
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
        osc::MeshCache& m_MeshCache;
        osc::Mesh m_SphereMesh = m_MeshCache.getSphereMesh();
        osc::Mesh m_CylinderMesh = m_MeshCache.getCylinderMesh();
        OpenSim::Model const& m_Model;
        OpenSim::ModelDisplayHints const& m_ModelDisplayHints = m_Model.getDisplayHints();
        bool m_ShowPathPoints = m_ModelDisplayHints.get_show_path_points();
        SimTK::SimbodyMatterSubsystem const& m_MatterSubsystem = m_Model.getSystem().getMatterSubsystem();
        SimTK::State const& m_State;
        osc::OpenSimDecorationOptions const& m_Opts;
        float m_FixupScaleFactor;
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& m_Out;
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

        glm::vec3 const p1 = TransformInGround(p2p.getBody1(), rs.getState()) * osc::ToVec3(p2p.getPoint1());
        glm::vec3 const p2 = TransformInGround(p2p.getBody2(), rs.getState()) * osc::ToVec3(p2p.getPoint2());

        float const radius = c_GeometryPathBaseRadius * rs.getFixupScaleFactor();
        osc::Transform const cylinderXform = osc::YToYCylinderToSegmentTransform({p1, p2}, radius);

        rs.consume(p2p, osc::SceneDecoration
        {
            rs.getCylinderMesh(),
            cylinderXform,
            osc::Color{0.7f, 0.7f, 0.7f, 1.0f},
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(
        RendererState& rs,
        OpenSim::Station const& s)
    {
        float radius = rs.getFixupScaleFactor() * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        osc::Transform xform;
        xform.position = osc::ToVec3(s.getLocationInGround(rs.getState()));
        xform.scale = {radius, radius, radius};

        rs.consume(s, osc::SceneDecoration
        {
            rs.getSphereMesh(),
            xform,
            osc::Color::red(),
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void HandleScapulothoracicJoint(
        RendererState& rs,
        OpenSim::ScapulothoracicJoint const& scapuloJoint)
    {
        osc::Transform t = TransformInGround(scapuloJoint.getParentFrame(), rs.getState());
        t.scale = osc::ToVec3(scapuloJoint.get_thoracic_ellipsoid_radii_x_y_z());

        rs.consume(scapuloJoint, osc::SceneDecoration
        {
            rs.getSphereMesh(),
            t,
            osc::Color{1.0f, 1.0f, 0.0f, 0.2f},
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
            osc::Transform t = TransformInGround(b, rs.getState());
            t.position = osc::TransformPoint(t, osc::ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            rs.consume(b, osc::SceneDecoration
            {
                rs.getSphereMesh(),
                t,
                osc::Color::black(),
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
        float const fixupScaleFactor = rs.getFixupScaleFactor();
        std::vector<osc::GeometryPathPoint> const pps = osc::GetAllPathPoints(muscle.getGeometryPath(), rs.getState());

        if (pps.empty())
        {
            // edge-case: there are no points in the muscle path
            return;
        }

        float const fiberUiRadius = GetMuscleSize(
            muscle,
            fixupScaleFactor,
            rs.getOptions().getMuscleSizingStyle()
        );
        float const tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        osc::Color const fiberColor = GetMuscleColor(
            muscle,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );
        osc::Color const tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f, 1.0f};

        osc::SceneDecoration fiberSpherePrototype =
        {
            rs.getSphereMesh(),
            osc::Transform{},
            fiberColor,
        };
        fiberSpherePrototype.transform.scale = {fiberUiRadius, fiberUiRadius, fiberUiRadius};

        osc::SceneDecoration tendonSpherePrototype{fiberSpherePrototype};
        tendonSpherePrototype.transform.scale = {tendonUiRadius, tendonUiRadius, tendonUiRadius};
        tendonSpherePrototype.color = tendonColor;

        auto emitTendonSphere = [&](glm::vec3 const& pos)
        {
            osc::SceneDecoration copy{tendonSpherePrototype};
            copy.transform.position = pos;
            rs.consume(muscle, std::move(copy));
        };
        auto emitTendonCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::YToYCylinderToSegmentTransform({p1, p2}, tendonUiRadius);

            rs.consume(muscle, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                tendonColor,
            });
        };
        auto emitFiberSphere = [&](glm::vec3 const& pos)
        {
            osc::SceneDecoration copy{fiberSpherePrototype};
            copy.transform.position = pos;
            rs.consume(muscle, std::move(copy));
        };
        auto emitFiberCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::YToYCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            rs.consume(muscle, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                fiberColor,
            });
        };

        if (pps.size() == 1)
        {
            // edge-case: the muscle is a single point in space: just emit a sphere
            //
            // (this really should never happen, but you never know)
            emitFiberSphere(pps.front().locationInGround);
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it
        auto tendonLen = static_cast<float>(muscle.getTendonLength(rs.getState()) * 0.5);
        tendonLen = std::clamp(tendonLen, 0.0f, tendonLen);
        auto fiberLen = static_cast<float>(muscle.getFiberLength(rs.getState()));
        fiberLen = std::clamp(fiberLen, 0.0f, fiberLen);
        float const fiberEnd = tendonLen + fiberLen;

        size_t i = 1;
        osc::GeometryPathPoint prevPoint = pps.front();
        float prevTraversalPos = 0.0f;

        // draw first tendon
        if (prevTraversalPos < tendonLen)
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.locationInGround);
        }
        while (i < pps.size() && prevTraversalPos < tendonLen)
        {
            // emit remaining tendon cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 const prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f)
            {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 tendonEnd = prevPoint.locationInGround + scaler * prevToPos;

                emitTendonCylinder(prevPoint.locationInGround, tendonEnd);
                emitTendonSphere(tendonEnd);

                prevPoint.locationInGround = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else
            {
                emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
                emitTendonSphere(point.locationInGround);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit first fiber sphere
            emitFiberSphere(prevPoint.locationInGround);
        }
        while (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit remaining fiber cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f)
            {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 fiberEndPos = prevPoint.locationInGround + scaler * prevToPos;

                emitFiberCylinder(prevPoint.locationInGround, fiberEndPos);
                emitFiberSphere(fiberEndPos);

                prevPoint.locationInGround = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else
            {
                emitFiberCylinder(prevPoint.locationInGround, point.locationInGround);
                emitFiberSphere(point.locationInGround);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw second tendon
        if (i < pps.size())
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.locationInGround);
        }
        while (i < pps.size())
        {
            // emit remaining fiber cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;

            emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
            emitTendonSphere(point.locationInGround);

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
        nonstd::span<osc::GeometryPathPoint const> points,
        float radius,
        osc::Color const& color)
    {
        if (points.empty())
        {
            // edge-case: there's no points to emit
            return;
        }

        // helper function: emits a sphere decoration
        auto const emitSphere = [&rs, &hittestTarget, radius, color](
            osc::GeometryPathPoint const& pp,
            glm::vec3 const& upDirection)
        {
            // ensure that user-defined path points are independently selectable (#425)
            OpenSim::Component const& c = pp.maybeUnderlyingUserPathPoint ?
                *pp.maybeUnderlyingUserPathPoint :
                hittestTarget;

            // ensure the sphere directionally tries to line up with the cylinders, to make
            // the "join" between the sphere and cylinders nicer (#593)
            osc::Transform t;
            t.scale *= radius;
            t.rotation = glm::normalize(glm::rotation(glm::vec3{0.0f, 1.0f, 0.0f}, upDirection));
            t.position = pp.locationInGround;

            rs.consume(c, osc::SceneDecoration
            {
                rs.getSphereMesh(),
                t,
                color,
            });
        };

        // helper function: emits a cylinder decoration between two points
        auto const emitCylinder = [&rs, &hittestTarget, radius, color](
            glm::vec3 const& p1,
            glm::vec3 const& p2)
        {
            osc::Transform const cylinderXform =
                osc::YToYCylinderToSegmentTransform({p1, p2}, radius);

            rs.consume(hittestTarget, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                color,
            });
        };

        // if required, draw first path point
        if (rs.getShowPathPoints())
        {
            osc::GeometryPathPoint const& firstPoint = points.front();
            glm::vec3 const& ppPos = firstPoint.locationInGround;
            glm::vec3 const direction = points.size() == 1 ?
                glm::vec3{0.0f, 1.0f, 0.0f} :
                glm::normalize(points[1].locationInGround - ppPos);

            emitSphere(firstPoint, direction);
        }

        // draw remaining cylinders and (if required) muscle path points
        for (size_t i = 1; i < points.size(); ++i)
        {
            osc::GeometryPathPoint const& point = points[i];

            glm::vec3 const& prevPos = points[i - 1].locationInGround;
            glm::vec3 const& curPos = point.locationInGround;

            emitCylinder(prevPos, curPos);

            // if required, draw path points
            if (rs.getShowPathPoints())
            {
                glm::vec3 const direction = glm::normalize(curPos - prevPos);
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
        std::vector<osc::GeometryPathPoint> const points =
            osc::GetAllPathPoints(musc.getGeometryPath(), rs.getState());

        float const radius = GetMuscleSize(
            musc,
            rs.getFixupScaleFactor(),
            rs.getOptions().getMuscleSizingStyle()
        );

        osc::Color const color = GetMuscleColor(
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

        std::vector<osc::GeometryPathPoint> const points = osc::GetAllPathPoints(gp, rs.getState());
        osc::Color const color = GetGeometryPathColor(gp, rs.getState());

        EmitPointBasedLine(rs, hittestTarget, points, c_GeometryPathBaseRadius, color);
    }

    void DrawLineOfActionArrow(
        RendererState& rs,
        OpenSim::Muscle const& muscle,
        osc::PointDirection const& loaPointDirection,
        osc::Color const& color)
    {
        float const fixupScaleFactor = rs.getFixupScaleFactor();

        osc::ArrowProperties p;
        p.worldspaceStart = loaPointDirection.point;
        p.worldspaceEnd = loaPointDirection.point + (fixupScaleFactor*0.1f)*loaPointDirection.direction;
        p.tipLength = (fixupScaleFactor*0.015f);
        p.headThickness = (fixupScaleFactor*0.01f);
        p.neckThickness = (fixupScaleFactor*0.006f);
        p.color = color;

        osc::DrawArrow(rs.updMeshCache(), p, [&muscle, &rs](osc::SceneDecoration&& d)
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
            if (std::optional<osc::LinesOfAction> const loas = osc::GetEffectiveLinesOfActionInGround(musc, rs.getState()))
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
            if (std::optional<osc::LinesOfAction> const loas = osc::GetAnatomicalLinesOfActionInGround(musc, rs.getState()))
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
        if (auto const* const musc = osc::GetOwner<OpenSim::Muscle>(gp))
        {
            // owner is a muscle, coerce selection "hit" to the muscle

            HandleLinesOfAction(rs, *musc);

            switch (rs.getOptions().getMuscleDecorationStyle())
            {
            case osc::MuscleDecorationStyle::FibersAndTendons:
                HandleMuscleFibersAndTendons(rs, *musc);
                return;
            case osc::MuscleDecorationStyle::Hidden:
                return;  // just don't generate them
            case osc::MuscleDecorationStyle::OpenSim:
            default:
                HandleMuscleOpenSimStyle(rs, *musc);
                return;
            }
        }
        else if (auto const* const pa = osc::GetOwner<OpenSim::PathActuator>(gp))
        {
            // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
            HandleGenericGeometryPath(rs, gp, *pa);
            return;
        }
        else if (auto const* const pathSpring = osc::GetOwner<OpenSim::PathSpring>(gp))
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
            osc::GetOwnerOr(frameGeometry, frameGeometry);

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
        std::optional<osc::ForcePoint> const maybeContact = osc::TryGetContactForceInGround(
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
        float const tipLength = 0.1f*glm::length((fixupScaleFactor*lenScale)*maybeContact->force);

        osc::ArrowProperties p;
        p.worldspaceStart = maybeContact->point;
        p.worldspaceEnd = maybeContact->point + (fixupScaleFactor*lenScale)*maybeContact->force;
        p.tipLength = tipLength;
        p.headThickness = fixupScaleFactor*baseRadius;
        p.neckThickness = fixupScaleFactor*baseRadius*0.6f;
        p.color = osc::Color::yellow();

        osc::DrawArrow(rs.updMeshCache(), p, [&hcf, &rs](osc::SceneDecoration&& d)
        {
            rs.consume(hcf, std::move(d));
        });
    }
}

void osc::GenerateModelDecorations(
    MeshCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSimDecorationOptions const& opts,
    float fixupScaleFactor,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
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

    for (OpenSim::Component const& c : model.getComponentList())
    {
        // handle OSC-specific decoration specializations, or fallback to generic
        // component decoration handling
        if (!osc::ShouldShowInUI(c))
        {
            continue;
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
    }
}

float osc::GetRecommendedScaleFactor(
    MeshCache& meshCache,
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
