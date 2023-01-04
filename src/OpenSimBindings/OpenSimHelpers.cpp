#include "OpenSimHelpers.hpp"

#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Transform.hpp"
#include "src/OpenSimBindings/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/MuscleSizingStyle.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/BodySet.h>
#include <OpenSim/Simulation/Model/ConstraintSet.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/ContactGeometrySet.h>
#include <OpenSim/Simulation/Model/ControllerSet.h>
#include <OpenSim/Simulation/Model/CoordinateSet.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/ForceSet.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/MarkerSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PathPointSet.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/Model/ProbeSet.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/Wrap/PathWrap.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <OpenSim/Simulation/Wrap/PathWrapSet.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <SimTKcommon.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

namespace osc { class Mesh; }

static osc::Transform TransformInGround(OpenSim::PhysicalFrame const& pf, SimTK::State const& st)
{
    return osc::ToTransform(pf.getTransformInGround(st));
}

// geometry rendering/handling support
namespace
{
    // helper: compute the decoration flags for a given component
    osc::SceneDecorationFlags ComputeFlags(
        OpenSim::Component const& c,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered)
    {
        osc::SceneDecorationFlags rv = osc::SceneDecorationFlags_CastsShadows;

        if (&c == selected)
        {
            rv |= osc::SceneDecorationFlags_IsSelected;
        }

        if (&c == hovered)
        {
            rv |= osc::SceneDecorationFlags_IsHovered;
        }

        OpenSim::Component const* ptr = osc::GetOwner(c);
        while (ptr)
        {
            if (ptr == selected)
            {
                rv |= osc::SceneDecorationFlags_IsChildOfSelected;
            }
            if (ptr == hovered)
            {
                rv |= osc::SceneDecorationFlags_IsChildOfHovered;
            }
            ptr = osc::GetOwner(*ptr);
        }

        return rv;
    }

    // simplification of a point in a geometry path
    struct GeometryPathPoint final {

        // set to be != nullptr if the point is associated with a (probably, user defined)
        // path point
        OpenSim::PathPoint const* maybePathPoint = nullptr;
        glm::vec3 location = {};

        explicit GeometryPathPoint(glm::vec3 const& location_) :
            location{location_}
        {
        }

        GeometryPathPoint(OpenSim::PathPoint const& pathPoint, glm::vec3 const& location_) :
            maybePathPoint{&pathPoint},
            location{location_}
        {
        }
    };

    // helper: returns path points in a GeometryPath as a sequence of 3D vectors
    std::vector<GeometryPathPoint> GetAllPathPoints(OpenSim::GeometryPath const& gp, SimTK::State const& st)
    {
        std::vector<GeometryPathPoint> rv;

        OpenSim::Array<OpenSim::AbstractPathPoint*> const& pps = gp.getCurrentPath(st);

        for (int i = 0; i < pps.getSize(); ++i)
        {
            if (OpenSim::PathWrapPoint const* pwp = dynamic_cast<OpenSim::PathWrapPoint const*>(pps[i]))
            {
                osc::Transform body2ground = osc::ToTransform(pwp->getParentFrame().getTransformInGround(st));
                OpenSim::Array<SimTK::Vec3> const& wrapPath = pwp->getWrapPath(st);

                for (int j = 0; j < wrapPath.getSize(); ++j)
                {
                    rv.emplace_back(body2ground * osc::ToVec3(wrapPath[j]));
                }
            }
            else if (OpenSim::PathPoint const* pp = dynamic_cast<OpenSim::PathPoint const*>(pps[i]))
            {
                rv.emplace_back(*pp, osc::ToVec3(pps[i]->getLocationInGround(st)));
            }
            else
            {
                rv.emplace_back(osc::ToVec3(pps[i]->getLocationInGround(st)));
            }
        }

        return rv;
    }

    // helper: calculates the radius of a muscle based on isometric force
    //
    // similar to how SCONE does it, so that users can compare between the two apps
    float GetSconeStyleAutomaticMuscleRadiusCalc(OpenSim::Muscle const& m)
    {
        float f = static_cast<float>(m.getMaxIsometricForce());
        float specificTension = 0.25e6f;  // magic number?
        float pcsa = f / specificTension;
        float widthFactor = 0.25f;
        return widthFactor * std::sqrt(pcsa / osc::fpi);
    }

    // returns value between [0.0f, 1.0f]
    float GetMuscleColorFactor(OpenSim::Muscle const& musc, SimTK::State const& st, osc::MuscleColoringStyle s)
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
            float nfl = static_cast<float>(musc.getNormalizedFiberLength(st));  // 1.0f == ideal length
            float fl = nfl - 1.0f;
            fl = std::abs(fl);
            fl = std::min(fl, 1.0f);
            return fl;
        }
        default:
            return 1.0f;
        }
    }

    // helper: returns the color a muscle should have, based on a variety of options (style, user-defined stuff in OpenSim, etc.)
    //
    // this is just a rough estimation of how SCONE is coloring things
    glm::vec4 GetMuscleColor(OpenSim::Muscle const& musc, SimTK::State const& st, osc::MuscleColoringStyle s)
    {
        if (s == osc::MuscleColoringStyle::OpenSim)
        {
            // use the same color that OpenSim emits (which is usually just activation-based, but might
            // change in the future)
            SimTK::Vec3 c = musc.getGeometryPath().getColor(st);
            return glm::vec4{osc::ToVec3(c), 1.0f};
        }
        else
        {
            // compute the color from the chosen color option
            glm::vec4 const zeroColor = {50.0f/255.0f, 50.0f/255.0f, 166.0f/255.0f, 1.0f};
            glm::vec4 const fullColor = {255.0f/255.0f, 25.0f/255.0f, 25.0f/255.0f, 1.0f};
            float const factor = GetMuscleColorFactor(musc, st, s);
            return zeroColor + factor * (fullColor - zeroColor);
        }
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(OpenSim::Muscle const& musc, float fixupScaleFactor, osc::MuscleSizingStyle s)
    {
        switch (s) {
        case osc::MuscleSizingStyle::SconePCSA:
            return GetSconeStyleAutomaticMuscleRadiusCalc(musc);
        case osc::MuscleSizingStyle::SconeNonPCSA:
            return 0.01f * fixupScaleFactor;
        case osc::MuscleSizingStyle::OpenSim:
        default:
            return 0.005f * fixupScaleFactor;
        }
    }

    // generic decoration handler for any `OpenSim::Component`
    void HandleComponent(OpenSim::Component const& c,
                         SimTK::State const& st,
                         OpenSim::ModelDisplayHints const& mdh,
                         SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                         osc::DecorativeGeometryHandler& handler)
    {
        {
            OSC_PERF("OpenSim::Component::generateDecorations(true, ...)");
            c.generateDecorations(true, mdh, st, geomList);
        }

        {
            OSC_PERF("(pump fixed decorations into OSC)");
            for (SimTK::DecorativeGeometry const& dg : geomList)
            {
                handler(dg);
            }
        }
        geomList.clear();

        {
            OSC_PERF("OpenSim::Component::generateDecorations(false, ...)");
            c.generateDecorations(false, mdh, st, geomList);
        }

        {
            OSC_PERF("(pump dynamic decorations into OSC)");
            for (SimTK::DecorativeGeometry const& dg : geomList)
            {
                handler(dg);
            }
        }
        geomList.clear();
    }

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void HandlePointToPointSpring(OpenSim::PointToPointSpring const& p2p,
                                  SimTK::State const& st,
                                  OpenSim::Component const* selected,
                                  OpenSim::Component const* hovered,
                                  float fixupScaleFactor,
                                  std::vector<osc::SceneDecoration>& out)
    {
        glm::vec3 p1 = TransformInGround(p2p.getBody1(), st) * osc::ToVec3(p2p.getPoint1());
        glm::vec3 p2 = TransformInGround(p2p.getBody2(), st) * osc::ToVec3(p2p.getPoint2());

        float radius = 0.005f * fixupScaleFactor;
        osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, radius);

        out.emplace_back(
            osc::App::singleton<osc::MeshCache>()->getCylinderMesh(),
            cylinderXform,
            glm::vec4{0.7f, 0.7f, 0.7f, 1.0f},
            p2p.getAbsolutePathString(),
            ComputeFlags(p2p, selected, hovered)
        );
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(OpenSim::Station const& s,
                       SimTK::State const& st,
                       OpenSim::Component const* selected,
                       OpenSim::Component const* hovered,
                       float fixupScaleFactor,
                       std::vector<osc::SceneDecoration>& out)
    {
        float radius = fixupScaleFactor * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        osc::Transform xform;
        xform.position = osc::ToVec3(s.getLocationInGround(st));
        xform.scale = {radius, radius, radius};

        out.emplace_back(
            osc::App::singleton<osc::MeshCache>()->getSphereMesh(),
            xform,
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
            s.getAbsolutePathString(),
            ComputeFlags(s, selected, hovered)
        );
    }

    void HandleScapulothoracicJoint(OpenSim::ScapulothoracicJoint const& j,
                                    SimTK::State const& st,
                                    OpenSim::Component const* selected,
                                    OpenSim::Component const* hovered,
                                    float fixupScaleFactor,
                                    std::vector<osc::SceneDecoration>& out)
    {
        osc::Transform t = osc::ToTransform(j.getParentFrame().getTransformInGround(st));
        t.scale = osc::ToVec3(j.get_thoracic_ellipsoid_radii_x_y_z());

        out.emplace_back(
            osc::App::singleton<osc::MeshCache>()->getSphereMesh(),
            t,
            glm::vec4{1.0f, 1.0f, 0.0f, 0.2f},
            j.getAbsolutePathString(),
            ComputeFlags(j, selected, hovered)
        );
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(OpenSim::Body const& b,
                    SimTK::State const& st,
                    float fixupScaleFactor,
                    OpenSim::Component const* selected,
                    OpenSim::Component const* hovered,
                    std::vector<osc::SceneDecoration>& out,
                    OpenSim::ModelDisplayHints const& mdh,
                    SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                    osc::DecorativeGeometryHandler& producer)
    {
        // bodies are drawn normally but *also* draw a center-of-mass sphere if they are
        // currently hovered
        if (&b == hovered && b.getMassCenter() != SimTK::Vec3{0.0, 0.0, 0.0})
        {
            float radius = fixupScaleFactor * 0.005f;
            osc::Transform t = TransformInGround(b, st);
            t.position = osc::TransformPoint(t, osc::ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            out.emplace_back(
                osc::App::singleton<osc::MeshCache>()->getSphereMesh(),
                t,
                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                b.getAbsolutePathString(),
                ComputeFlags(b, selected, hovered)
            );
        }

        HandleComponent(b, st, mdh, geomList, producer);
    }

    // OSC-specific decoration handler for `OpenSim::Muscle` ("SCONE"-style: i.e. tendons + muscle)
    void HandleMuscleSconeStyle(osc::CustomDecorationOptions const& opts,
                                OpenSim::Muscle const& muscle,
                                SimTK::State const& st,
                                OpenSim::Component const* selected,
                                OpenSim::Component const* hovered,
                                float fixupScaleFactor,
                                OpenSim::ModelDisplayHints const&,
                                std::vector<osc::SceneDecoration>& out)
    {
        std::vector<GeometryPathPoint> const pps = GetAllPathPoints(muscle.getGeometryPath(), st);
        std::string const muscleAbsPath = muscle.getAbsolutePathString();

        if (pps.empty())
        {
            // edge-case: there are no points in the muscle path
            return;
        }

        float const fiberUiRadius = GetMuscleSize(muscle, fixupScaleFactor, opts.getMuscleSizingStyle());
        float const tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        glm::vec4 const fiberColor = GetMuscleColor(muscle, st, opts.getMuscleColoringStyle());
        glm::vec4 const tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f, 1.0f};

        osc::SceneDecorationFlags flags = ComputeFlags(muscle, selected, hovered);

        osc::SceneDecoration fiberSpherePrototype =
        {
            osc::App::singleton<osc::MeshCache>()->getSphereMesh(),
            osc::Transform{},
            fiberColor,
            muscleAbsPath,
            flags,
        };
        fiberSpherePrototype.transform.scale = {fiberUiRadius, fiberUiRadius, fiberUiRadius};

        osc::SceneDecoration tendonSpherePrototype{fiberSpherePrototype};
        tendonSpherePrototype.transform.scale = {tendonUiRadius, tendonUiRadius, tendonUiRadius};
        tendonSpherePrototype.color = tendonColor;

        auto emitTendonSphere = [&](glm::vec3 const& pos)
        {
            out.emplace_back(tendonSpherePrototype).transform.position = pos;
        };
        auto emitTendonCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, tendonUiRadius);

            out.emplace_back(
                osc::App::singleton<osc::MeshCache>()->getCylinderMesh(),
                cylinderXform,
                tendonColor,
                muscleAbsPath,
                flags
            );
        };
        auto emitFiberSphere = [&](glm::vec3 const& pos)
        {
            out.emplace_back(fiberSpherePrototype).transform.position = pos;
        };
        auto emitFiberCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            out.emplace_back(
                osc::App::singleton<osc::MeshCache>()->getCylinderMesh(),
                cylinderXform,
                fiberColor,
                muscleAbsPath,
                flags
            );
        };

        if (pps.size() == 1)
        {
            // edge-case: the muscle is a single point in space: just emit a sphere
            //
            // (this really should never happen, but you never know)
            emitFiberSphere(pps.front().location);
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it
        out.reserve(out.size() + (2*pps.size() - 1) + 6);

        float tendonLen = static_cast<float>(muscle.getTendonLength(st) * 0.5);
        tendonLen = std::clamp(tendonLen, 0.0f, tendonLen);
        float fiberLen = static_cast<float>(muscle.getFiberLength(st));
        fiberLen = std::clamp(fiberLen, 0.0f, fiberLen);
        float const fiberEnd = tendonLen + fiberLen;

        size_t i = 1;
        GeometryPathPoint prevPoint = pps.front();
        float prevTraversalPos = 0.0f;

        // draw first tendon
        if (prevTraversalPos < tendonLen)
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.location);
        }
        while (i < pps.size() && prevTraversalPos < tendonLen)
        {
            // emit remaining tendon cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            glm::vec3 const prevToPos = point.location - prevPoint.location;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f)
            {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 tendonEnd = prevPoint.location + scaler * prevToPos;

                emitTendonCylinder(prevPoint.location, tendonEnd);
                emitTendonSphere(tendonEnd);

                prevPoint.location = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else
            {
                emitTendonCylinder(prevPoint.location, point.location);
                emitTendonSphere(point.location);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit first fiber sphere
            emitFiberSphere(prevPoint.location);
        }
        while (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit remaining fiber cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.location - prevPoint.location;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f)
            {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 fiberEndPos = prevPoint.location + scaler * prevToPos;

                emitFiberCylinder(prevPoint.location, fiberEndPos);
                emitFiberSphere(fiberEndPos);

                prevPoint.location = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else
            {
                emitFiberCylinder(prevPoint.location, point.location);
                emitFiberSphere(point.location);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw second tendon
        if (i < pps.size())
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.location);
        }
        while (i < pps.size())
        {
            // emit remaining fiber cylinder + spheres

            GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.location - prevPoint.location;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;

            emitTendonCylinder(prevPoint.location, point.location);
            emitTendonSphere(point.location);

            i++;
            prevPoint = point;
            prevTraversalPos = traversalPos;
        }
    }

    // OSC-specific decoration handler for `OpenSim::Muscle`
    void HandleMuscleOpenSimStyle(osc::CustomDecorationOptions const& opts,
                                  OpenSim::Muscle const& musc,
                                  SimTK::State const& st,
                                  OpenSim::Component const* selected,
                                  OpenSim::Component const* hovered,
                                  float fixupScaleFactor,
                                  OpenSim::Component const**,
                                  OpenSim::ModelDisplayHints const& mdh,
                                  SimTK::Array_<SimTK::DecorativeGeometry>&,
                                  osc::DecorativeGeometryHandler&,
                                  std::vector<osc::SceneDecoration>& out)
    {
        osc::SceneDecorationFlags const flags = ComputeFlags(musc, selected, hovered);
        std::vector<GeometryPathPoint> const pps = GetAllPathPoints(musc.getGeometryPath(), st);
        std::string const absPath = musc.getAbsolutePathString();

        if (pps.empty())
        {
            return;
        }

        float const fiberUiRadius = GetMuscleSize(musc, fixupScaleFactor, opts.getMuscleSizingStyle());
        glm::vec4 const fiberColor = GetMuscleColor(musc, st, opts.getMuscleColoringStyle());

        auto emitSphere = [&](GeometryPathPoint const& pp)
        {
            // ensure that user-defined path points are independently selectable (#425)
            //
            // TODO: SCONE-style etc. should also support this
            OpenSim::Component const& c = pp.maybePathPoint ? *pp.maybePathPoint : static_cast<OpenSim::Component const&>(musc);
            osc::SceneDecorationFlags const sphereFlags = ComputeFlags(c, selected, hovered);

            osc::Transform t;
            t.scale *= fiberUiRadius;
            t.position = pp.location;

            out.emplace_back(
                osc::App::singleton<osc::MeshCache>()->getSphereMesh(),
                t,
                fiberColor,
                pp.maybePathPoint ? pp.maybePathPoint->getAbsolutePathString() : absPath,
                sphereFlags
            );
        };

        auto emitCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            out.emplace_back(
                osc::App::singleton<osc::MeshCache>()->getCylinderMesh(),
                cylinderXform,
                fiberColor,
                absPath,
                flags
            );
        };

        if (mdh.get_show_path_points())
        {
            emitSphere(pps.front());
        }
        for (std::size_t i = 1; i < pps.size(); ++i)
        {
            emitCylinder(pps[i - 1].location, pps[i].location);

            if (mdh.get_show_path_points())
            {
                emitSphere(pps[i]);
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(osc::CustomDecorationOptions const& opts,
                            OpenSim::GeometryPath const& gp,
                            SimTK::State const& st,
                            OpenSim::Component const* selected,
                            OpenSim::Component const* hovered,
                            float fixupScaleFactor,
                            OpenSim::Component const** currentComponent,
                            OpenSim::ModelDisplayHints const& mdh,
                            SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                            osc::DecorativeGeometryHandler& producer,
                            std::vector<osc::SceneDecoration>& out)
    {
        // even custom muscle decoration implementations *must* obey the visibility flag on `GeometryPath` (#414)
        if (!gp.get_Appearance().get_visible())
        {
            return;
        }

        if (gp.hasOwner())
        {
            // the `GeometryPath` has a owner, which might be a muscle or path actuator

            if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&gp.getOwner()); musc)
            {
                // owner is a muscle, coerce selection "hit" to the muscle
                *currentComponent = musc;

                switch (opts.getMuscleDecorationStyle())
                {
                case osc::MuscleDecorationStyle::Scone:
                    HandleMuscleSconeStyle(opts, *musc, st, selected, hovered, fixupScaleFactor, mdh, out);
                    return;
                case osc::MuscleDecorationStyle::Hidden:
                    return;  // just don't generate them
                case osc::MuscleDecorationStyle::OpenSim:
                default:
                    HandleMuscleOpenSimStyle(opts, *musc, st, selected, hovered, fixupScaleFactor, currentComponent, mdh, geomList, producer, out);
                    return;
                }
            }
            else if (auto const* pa = dynamic_cast<OpenSim::PathActuator const*>(&gp.getOwner()); pa)
            {
                // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
                *currentComponent = pa;

                // but render it as-normal
                HandleComponent(gp, st, mdh, geomList, producer);
                return;
            }
            else
            {
                // it's a path in some non-muscular context
                HandleComponent(gp, st, mdh, geomList, producer);
                return;
            }
        }
        else
        {
            // it's a standalone path that's not part of a muscle
            HandleComponent(gp, st, mdh, geomList, producer);
            return;
        }
    }

    void HandleFrameGeometry(
        osc::CustomDecorationOptions const& opts,
        OpenSim::FrameGeometry const& frameGeometry,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        OpenSim::Component const** currentComponent,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
        osc::DecorativeGeometryHandler& producer,
        std::vector<osc::SceneDecoration>& out)
    {
        if (frameGeometry.hasOwner())
        {
            // promote current component to the parent of the frame geometry, because
            // a user is probably more interested in the thing the frame geometry
            // represents (e.g. an offset frame) than the geometry itself (#506)
            *currentComponent = &frameGeometry.getOwner();
        }
        HandleComponent(frameGeometry, st, mdh, geomList, producer);
    }

    // a class that is called whenever the SimTK backend emits `DecorativeGeometry`
    class OpenSimDecorationConsumer final : public osc::DecorationConsumer {
    public:
        OpenSimDecorationConsumer(osc::VirtualConstModelStatePair const* msp,
                                  std::vector<osc::SceneDecoration>* out,
                                  OpenSim::Component const** currentComponent) :
            m_Msp{std::move(msp)},
            m_Out{std::move(out)},
            m_CurrentComponent{std::move(currentComponent)}
        {
        }

        void operator()(osc::Mesh const& mesh, osc::Transform const& t, glm::vec4 const& color) override
        {
            std::string absPath = (*m_CurrentComponent) ? (*m_CurrentComponent)->getAbsolutePathString() : std::string{};
            m_Out->emplace_back(mesh, t, color, std::move(absPath), ComputeFlags(**m_CurrentComponent, m_Selected, m_Hovered));
        }

    private:
        osc::VirtualConstModelStatePair const* m_Msp;
        OpenSim::Component const* m_Selected = m_Msp->getSelected();
        OpenSim::Component const* m_Hovered = m_Msp->getHovered();
        std::vector<osc::SceneDecoration>* m_Out;
        OpenSim::Component const** m_CurrentComponent;
    };

    // generates a sequence of OSC decoration from an OpenSim model + state
    void GenerateDecorationEls(osc::VirtualConstModelStatePair const& msp,
                               osc::CustomDecorationOptions const& opts,
                               std::vector<osc::SceneDecoration>& out)
    {
        out.clear();

        // assumed to be valid during the decoration generation
        std::shared_ptr<osc::MeshCache> const meshCache = osc::App::singleton<osc::MeshCache>();
        OpenSim::Model const& model = msp.getModel();
        SimTK::State const& state = msp.getState();
        OpenSim::Component const* selected = msp.getSelected();
        OpenSim::Component const* hovered = msp.getHovered();
        float fixupScaleFactor = msp.getFixupScaleFactor();
        OpenSim::ModelDisplayHints const& mdh = msp.getModel().getDisplayHints();

        // gets called whenever OpenSim is emitting stuff via `generateDecorations`
        OpenSim::Component const* currentComponent = nullptr;
        OpenSimDecorationConsumer consumer{&msp, &out, &currentComponent};

        // generates mesh/object instances for each SimTK::DecorativeGeometry it's given
        osc::DecorativeGeometryHandler producer
        {
            *meshCache,
            model.getSystem().getMatterSubsystem(),
            state,
            fixupScaleFactor,
            consumer
        };

        SimTK::Array_<SimTK::DecorativeGeometry> geomList;
        for (OpenSim::Component const& c : model.getComponentList())
        {
            if (!osc::ShouldShowInUI(c))
            {
                continue;
            }

            currentComponent = &c;

            // handle OSC-specific decoration specializations, or fallback to generic
            // component decoration handling
            if (auto const* p2p = dynamic_cast<OpenSim::PointToPointSpring const*>(&c))
            {
                HandlePointToPointSpring(*p2p, state, selected, hovered, fixupScaleFactor, out);
            }
            else if (typeid(c) == typeid(OpenSim::Station))  // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            {
                HandleStation(static_cast<OpenSim::Station const&>(c), state, selected, hovered, fixupScaleFactor, out);
            }
            else if (auto const* scapulo = dynamic_cast<OpenSim::ScapulothoracicJoint const*>(&c); scapulo && opts.getShouldShowScapulo())
            {
                HandleScapulothoracicJoint(*scapulo, state, selected, hovered, fixupScaleFactor, out);
            }
            else if (auto const* body = dynamic_cast<OpenSim::Body const*>(&c))
            {
                HandleBody(*body, state, fixupScaleFactor, selected, hovered, out, mdh, geomList, producer);
            }
            else if (auto const* gp = dynamic_cast<OpenSim::GeometryPath const*>(&c))
            {
                HandleGeometryPath(opts, *gp, state, selected, hovered, fixupScaleFactor, &currentComponent, mdh, geomList, producer, out);
            }
            else if (auto const* fg = dynamic_cast<OpenSim::FrameGeometry const*>(&c))
            {
                HandleFrameGeometry(opts, *fg, state, selected, hovered, fixupScaleFactor, &currentComponent, mdh, geomList, producer, out);
            }
            else
            {
                // generic handler
                HandleComponent(c, state, mdh, geomList, producer);
            }
        }
    }

    // try to delete an item from an OpenSim::Set
    //
    // returns `true` if the item was found and deleted; otherwise, returns `false`
    template<typename T, typename TSetBase = OpenSim::Object>
    bool TryDeleteItemFromSet(OpenSim::Set<T, TSetBase>& set, T const* item)
    {
        for (int i = 0; i < set.getSize(); ++i)
        {
            if (&set.get(i) == item)
            {
                set.remove(i);
                return true;
            }
        }
        return false;
    }
}

static bool IsConnectedViaSocketTo(OpenSim::Component& c, OpenSim::Component const& other)
{
    for (std::string const& socketName : c.getSocketNames())
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(socketName);
        if (sock.isConnected() && &sock.getConnecteeAsObject() == &other)
        {
            return true;
        }
    }
    return false;
}

static std::vector<OpenSim::Component*> GetAnyComponentsConnectedViaSocketTo(OpenSim::Component& root, OpenSim::Component const& other)
{
    std::vector<OpenSim::Component*> rv;

    if (IsConnectedViaSocketTo(root, other))
    {
        rv.push_back(&root);
    }

    for (OpenSim::Component& c : root.updComponentList())
    {
        if (IsConnectedViaSocketTo(c, other))
        {
            rv.push_back(&c);
        }
    }

    return rv;
}


// public API

bool osc::IsConcreteClassNameLexographicallyLowerThan(OpenSim::Component const& a, OpenSim::Component const& b)
{
    return a.getConcreteClassName() < b.getConcreteClassName();
}

bool osc::IsNameLexographicallyLowerThan(OpenSim::Component const& a, OpenSim::Component const& b)
{
    return a.getName() < b.getName();
}

OpenSim::Component* osc::UpdOwner(OpenSim::Component& c)
{
    return c.hasOwner() ? const_cast<OpenSim::Component*>(&c.getOwner()) : nullptr;
}

OpenSim::Component const* osc::GetOwner(OpenSim::Component const& c)
{
    return c.hasOwner() ? &c.getOwner() : nullptr;
}


int osc::DistanceFromRoot(OpenSim::Component const& c)
{
    OpenSim::Component const* p = &c;
    int dist = 0;

    while (p->hasOwner())
    {
        ++dist;
        p = &p->getOwner();
    }

    return dist;
}

OpenSim::ComponentPath const& osc::GetEmptyComponentPath()
{
    static OpenSim::ComponentPath p;
    return p;
}

OpenSim::ComponentPath const& osc::GetRootComponentPath()
{
    static OpenSim::ComponentPath p{"/"};
    return p;
}

bool osc::IsEmpty(OpenSim::ComponentPath const& cp)
{
    return cp == GetEmptyComponentPath();
}

void osc::Clear(OpenSim::ComponentPath& cp)
{
    cp = GetEmptyComponentPath();
}

std::vector<OpenSim::Component const*> osc::GetPathElements(OpenSim::Component const& c)
{
    std::vector<OpenSim::Component const*> rv;
    rv.reserve(DistanceFromRoot(c));

    OpenSim::Component const* p = &c;
    rv.push_back(p);

    while (p->hasOwner())
    {
        p = &p->getOwner();
        rv.push_back(p);
    }

    std::reverse(rv.begin(), rv.end());

    return rv;
}

bool osc::IsInclusiveChildOf(OpenSim::Component const* parent, OpenSim::Component const* c)
{
    if (!c)
    {
        return false;
    }

    if (!parent)
    {
        return false;
    }

    for (;;)
    {
        if (c == parent)
        {
            return true;
        }

        if (!c->hasOwner())
        {
            return false;
        }

        c = &c->getOwner();
    }
}

OpenSim::Component const* osc::IsInclusiveChildOf(nonstd::span<OpenSim::Component const*> parents, OpenSim::Component const* c)
{
    while (c)
    {
        for (size_t i = 0; i < parents.size(); ++i)
        {
            if (c == parents[i])
            {
                return parents[i];
            }
        }
        c = c->hasOwner() ? &c->getOwner() : nullptr;
    }
    return nullptr;
}

OpenSim::Component const* osc::FindFirstAncestorInclusive(OpenSim::Component const* c, bool(*pred)(OpenSim::Component const*))
{
    while (c)
    {
        if (pred(c))
        {
            return c;
        }
        c = c->hasOwner() ? &c->getOwner() : nullptr;
    }
    return nullptr;
}

std::vector<OpenSim::Coordinate const*> osc::GetCoordinatesInModel(OpenSim::Model const& model)
{
    std::vector<OpenSim::Coordinate const*> rv;
    GetCoordinatesInModel(model, rv);
    return rv;
}

void osc::GetCoordinatesInModel(OpenSim::Model const& m,
                                std::vector<OpenSim::Coordinate const*>& out)
{
    OpenSim::CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));

    for (int i = 0; i < len; ++i)
    {
        out.push_back(&s[i]);
    }
}

float osc::ConvertCoordValueToDisplayValue(OpenSim::Coordinate const& c, double v)
{
    float rv = static_cast<float>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = glm::degrees(rv);
    }

    return rv;
}

double osc::ConvertCoordDisplayValueToStorageValue(OpenSim::Coordinate const& c, float v)
{
    double rv = static_cast<double>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = glm::radians(rv);
    }

    return rv;
}

osc::CStringView osc::GetCoordDisplayValueUnitsString(OpenSim::Coordinate const& c)
{
    switch (c.getMotionType()) {
    case OpenSim::Coordinate::MotionType::Translational:
        return "m";
    case OpenSim::Coordinate::MotionType::Rotational:
        return "deg";
    default:
        return "";
    }
}

std::vector<std::string> osc::GetSocketNames(OpenSim::Component const& c)
{
    // const_cast is necessary because `getSocketNames` is somehow not-`const`
    return const_cast<OpenSim::Component&>(c).getSocketNames();
}

std::vector<OpenSim::AbstractSocket const*> osc::GetAllSockets(OpenSim::Component const& c)
{
    std::vector<OpenSim::AbstractSocket const*> rv;

    for (std::string const& name : GetSocketNames(c))
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        rv.push_back(&sock);
    }

    return rv;
}

OpenSim::Component const* osc::FindComponent(OpenSim::Component const& c, OpenSim::ComponentPath const& cp)
{
    if (IsEmpty(cp))
    {
        return nullptr;
    }

    try
    {
        return &c.getComponent(cp);
    }
    catch (OpenSim::Exception const&)
    {
        return nullptr;
    }
}

OpenSim::Component* osc::FindComponentMut(OpenSim::Component& c, OpenSim::ComponentPath const& cp)
{
    return const_cast<OpenSim::Component*>(FindComponent(c, cp));
}

bool osc::ContainsComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
{
    return FindComponent(root, cp);
}

OpenSim::AbstractSocket const* osc::FindSocket(OpenSim::Component const& c, std::string const& name)
{
    try
    {
        return &c.getSocket(name);
    }
    catch (OpenSim::SocketNotFound const&)
    {
        return nullptr;  // :(
    }
}

OpenSim::AbstractSocket* osc::FindSocketMut(OpenSim::Component& c, std::string const& name)
{
    try
    {
        return &c.updSocket(name);
    }
    catch (OpenSim::SocketNotFound const&)
    {
        return nullptr;  // :(
    }
}

bool osc::IsAbleToConnectTo(OpenSim::AbstractSocket const& s, OpenSim::Component const& c)
{
    // yes, this is very very bad

    std::unique_ptr<OpenSim::AbstractSocket> copy{s.clone()};
    try
    {
        copy->connect(c);
        return true;
    }
    catch (OpenSim::Exception const&)
    {
        return false;
    }
}

OpenSim::AbstractProperty* osc::FindPropertyMut(OpenSim::Component& c, std::string const& name)
{
    return c.hasProperty(name) ? &c.updPropertyByName(name) : nullptr;
}

OpenSim::AbstractOutput const* osc::FindOutput(OpenSim::Component const& c, std::string const& outputName)
{
    OpenSim::AbstractOutput const* rv = nullptr;
    try
    {
        rv = &c.getOutput(outputName);
    }
    catch (OpenSim::Exception const&)
    {
        // OpenSim, innit
    }
    return rv;
}

OpenSim::AbstractOutput const* osc::FindOutput(OpenSim::Component const& root,
                                               OpenSim::ComponentPath const& path,
                                               std::string const& outputName)
{
    OpenSim::Component const* c = FindComponent(root, path);
    return c ? FindOutput(*c, outputName) : nullptr;
}

bool osc::HasInputFileName(OpenSim::Model const& m)
{
    std::string const& name = m.getInputFileName();
    return !name.empty() && name != "Unassigned";
}

std::filesystem::path osc::TryFindInputFile(OpenSim::Model const& m)
{
    if (!HasInputFileName(m))
    {
        return {};
    }

    std::filesystem::path p{m.getInputFileName()};

    if (!std::filesystem::exists(p))
    {
        return {};
    }

    return p;
}

std::optional<std::filesystem::path> osc::FindGeometryFileAbsPath(
    OpenSim::Model const& model,
    OpenSim::Mesh const& mesh)
{
    // this implementation is designed to roughly mimic how OpenSim::Mesh::extendFinalizeFromProperties works

    std::string const& fileProp = mesh.get_mesh_file();
    std::filesystem::path const filePropPath{fileProp};

    bool isAbsolute = filePropPath.is_absolute();
    SimTK::Array_<std::string> attempts;
    bool const found = OpenSim::ModelVisualizer::findGeometryFile(
        model,
        fileProp,
        isAbsolute,
        attempts
    );

    if (!found || attempts.empty())
    {
        return std::nullopt;
    }

    return std::optional<std::filesystem::path>{std::filesystem::absolute({attempts.back()})};
}

bool osc::ShouldShowInUI(OpenSim::Component const& c)
{
    if (Is<OpenSim::PathWrapPoint>(c))
    {
        return false;
    }
    else if (Is<OpenSim::Station>(c) && c.hasOwner() && DerivesFrom<OpenSim::PathPoint>(c.getOwner()))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool osc::TryDeleteComponentFromModel(OpenSim::Model& m, OpenSim::Component& c)
{
    OpenSim::Component* owner = osc::UpdOwner(c);

    if (!owner)
    {
        log::error("cannot delete %s: it has no owner", c.getName().c_str());
        return false;
    }

    if (&c.getRoot() != &m)
    {
        log::error("cannot delete %s: it is not owned by the provided model");
        return false;
    }

    // check if anything connects to the component via a socket
    if (auto connectees = GetAnyComponentsConnectedViaSocketTo(m, c); !connectees.empty())
    {
        std::stringstream ss;
        char const* delim = "";
        for (OpenSim::Component const* connectee : connectees)
        {
            ss << delim << connectee->getName();
            delim = ", ";
        }
        log::error("cannot delete %s: the following components connect to it via sockets: %s", c.getName().c_str(), std::move(ss).str().c_str());
        return false;
    }

    // BUG/HACK: check if any path wraps connect to the component
    //
    // this is because the wrapping code isn't using sockets :< - this should be
    // fixed in OpenSim itself
    for (OpenSim::PathWrap const& pw : m.getComponentList<OpenSim::PathWrap>())
    {
        if (pw.getWrapObject() == &c)
        {
            log::error("cannot delete %s: it is used in a path wrap (%s)", c.getName().c_str(), pw.getAbsolutePathString().c_str());
            return false;
        }
    }

    // at this point we know that it's *technically* feasible to delete the component
    // from the model without breaking sockets etc., so now we use heuristics to figure
    // out how to do that

    bool rv = false;

    if (auto* js = dynamic_cast<OpenSim::JointSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*js, dynamic_cast<OpenSim::Joint*>(&c));
    }
    else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(owner))
    {
        rv = TryDeleteItemFromSet(*bs, dynamic_cast<OpenSim::Body*>(&c));
    }
    else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*wos, dynamic_cast<OpenSim::WrapObject*>(&c));
    }
    else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*cs, dynamic_cast<OpenSim::Controller*>(&c));
    }
    else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*conss, dynamic_cast<OpenSim::Constraint*>(&c));
    }
    else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*fs, dynamic_cast<OpenSim::Force*>(&c));
    }
    else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*ms, dynamic_cast<OpenSim::Marker*>(&c));
    }
    else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs)
    {
        rv = TryDeleteItemFromSet(*cgs, dynamic_cast<OpenSim::ContactGeometry*>(&c));
    }
    else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner))
    {
        rv = TryDeleteItemFromSet(*ps, dynamic_cast<OpenSim::Probe*>(&c));
    }
    else if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(owner))
    {
        if (auto* app = dynamic_cast<OpenSim::AbstractPathPoint*>(&c))
        {
            rv = TryDeleteItemFromSet(gp->updPathPointSet(), app);
        }
        else if (auto* pw = dynamic_cast<OpenSim::PathWrap*>(&c))
        {
            rv = TryDeleteItemFromSet(gp->updWrapSet(), pw);
        }
    }
    else if (auto* geom = FindAncestorWithTypeMut<OpenSim::Geometry>(&c))
    {
        // delete an OpenSim::Geometry from its owning OpenSim::Frame

        if (auto* frame = FindAncestorWithTypeMut<OpenSim::Frame>(geom))
        {
            // its owner is a frame, which holds the geometry in a list property

            // make a copy of the property containing the geometry and
            // only copy over the not-deleted geometry into the copy
            //
            // this is necessary because OpenSim::Property doesn't seem
            // to support list element deletion, but does support full
            // assignment

            OpenSim::ObjectProperty<OpenSim::Geometry>& prop =
                static_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(frame->updProperty_attached_geometry());

            std::unique_ptr<OpenSim::ObjectProperty<OpenSim::Geometry>> copy{prop.clone()};
            copy->clear();
            for (int i = 0; i < prop.size(); ++i)
            {
                OpenSim::Geometry& g = prop[i];
                if (&g != geom)
                {
                    copy->adoptAndAppendValue(g.clone());
                }
            }

            prop.assign(*copy);

            rv = true;
        }
    }

    if (!rv)
    {
        osc::log::error("cannot delete %s: OpenSim Creator doesn't know how to delete a %s from its parent (maybe it can't?)", c.getName().c_str(), c.getConcreteClassName().c_str());
    }

    return rv;
}

void osc::GenerateModelDecorations(VirtualConstModelStatePair const& p,
                                   std::vector<SceneDecoration>& out,
                                   CustomDecorationOptions const& opts)
{
    out.clear();

    {
        OSC_PERF("scene generation");
        GenerateDecorationEls(p, opts, out);
    }
}

void osc::GenerateModelDecorations(VirtualConstModelStatePair const& p,
                                   std::vector<SceneDecoration>& out)
{
    GenerateModelDecorations(p, out, CustomDecorationOptions{});
}

void osc::CopyCommonJointProperties(OpenSim::Joint const& src, OpenSim::Joint& dest)
{
    dest.setName(src.getName());

    // copy owned frames
    dest.updProperty_frames().assign(src.getProperty_frames());

    // copy parent frame socket *path* (note: don't use connectSocket, pointers are evil in model manipulations)
    dest.updSocket("parent_frame").setConnecteePath(src.getSocket("parent_frame").getConnecteePath());

    // copy child socket *path* (note: don't use connectSocket, pointers are evil in model manipulations)
    dest.updSocket("child_frame").setConnecteePath(src.getSocket("child_frame").getConnecteePath());
}

bool osc::DeactivateAllWrapObjectsIn(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>())
    {
        for (int i = 0; i < wos.getSize(); ++i)
        {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
            rv = rv || true;
        }
    }
    return rv;
}

bool osc::ActivateAllWrapObjectsIn(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>())
    {
        for (int i = 0; i < wos.getSize(); ++i)
        {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
            rv = rv || true;
        }
    }
    return rv;
}

void osc::AddComponentToModel(OpenSim::Model& m, std::unique_ptr<OpenSim::Component> c)
{
    if (!c)
    {
        return;  // paranoia
    }
    else if (dynamic_cast<OpenSim::Body*>(c.get()))
    {
        m.addBody(static_cast<OpenSim::Body*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Joint*>(c.get()))
    {
        m.addJoint(static_cast<OpenSim::Joint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Constraint*>(c.get()))
    {
        m.addConstraint(static_cast<OpenSim::Constraint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Force*>(c.get()))
    {
        m.addForce(static_cast<OpenSim::Force*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Probe*>(c.get()))
    {
        m.addProbe(static_cast<OpenSim::Probe*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::ContactGeometry*>(c.get()))
    {
        m.addContactGeometry(static_cast<OpenSim::ContactGeometry*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Marker*>(c.get()))
    {
        m.addMarker(static_cast<OpenSim::Marker*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Controller*>(c.get()))
    {
        m.addController(static_cast<OpenSim::Controller*>(c.release()));
    }
    else
    {
        m.addComponent(c.release());
    }

    m.finalizeConnections();  // necessary, because adding it may have created a new (not finalized) connection
}

float osc::GetRecommendedScaleFactor(VirtualConstModelStatePair const& p)
{
    // generate decorations as if they were empty-sized and union their
    // AABBs to get an idea of what the "true" scale of the model probably
    // is (without the model containing oversized frames, etc.)
    std::vector<SceneDecoration> ses;
    GenerateModelDecorations(p, ses);

    if (ses.empty())
    {
        return 1.0f;
    }

    AABB aabb = GetWorldspaceAABB(ses[0]);
    for (size_t i = 1; i < ses.size(); ++i)
    {
        aabb = Union(aabb, GetWorldspaceAABB(ses[i]));
    }

    float longest = LongestDim(aabb);
    float rv = 1.0f;

    while (longest < 0.1)
    {
        longest *= 10.0f;
        rv /= 10.0f;
    }

    return rv;
}

std::unique_ptr<osc::UndoableModelStatePair> osc::LoadOsimIntoUndoableModel(std::filesystem::path p)
{
    return std::make_unique<osc::UndoableModelStatePair>(p);
}

void osc::InitializeModel(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeModel");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)
    model.clearConnections();        // clears any potentially stale pointers that can be retained by OpenSim::Socket<T> (see #263)
    model.buildSystem();             // creates a new underlying physics system
}

SimTK::State& osc::InitializeState(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeState");
    SimTK::State& state = model.initializeState();  // creates+returns a new working state
    model.equilibrateMuscles(state);
    model.realizeDynamics(state);
    return state;
}

std::optional<int> osc::FindJointInParentJointSet(OpenSim::Joint const& joint)
{
    auto const* parentJointset =
        joint.hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&joint.getOwner()) : nullptr;

    if (!parentJointset)
    {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return std::nullopt;
    }

    OpenSim::JointSet const& js = *parentJointset;

    for (int i = 0; i < js.getSize(); ++i)
    {
        OpenSim::Joint const* j = &js[i];
        if (j == &joint)
        {
            return i;
        }
    }
    return std::nullopt;
}

std::string osc::GetRecommendedDocumentName(osc::UndoableModelStatePair const& uim)
{
    if (uim.hasFilesystemLocation())
    {
        return uim.getFilesystemPath().filename().string();
    }
    else
    {
        return "untitled.osim";
    }
}

std::string osc::GetDisplayName(OpenSim::Geometry const& g)
{
    if (OpenSim::Mesh const* mesh = dynamic_cast<OpenSim::Mesh const*>(&g); mesh)
    {
        return mesh->getGeometryFilename();
    }
    else
    {
        return g.getConcreteClassName();
    }
}

char const* osc::GetMotionTypeDisplayName(OpenSim::Coordinate const& c)
{
    switch (c.getMotionType()) {
    case OpenSim::Coordinate::MotionType::Rotational:
        return "Rotational";
    case OpenSim::Coordinate::MotionType::Translational:
        return "Translational";
    case OpenSim::Coordinate::MotionType::Coupled:
        return "Coupled";
    default:
        return "Unknown";
    }
}

bool osc::TrySetAppearancePropertyIsVisibleTo(OpenSim::Component& c, bool v)
{
    if (!c.hasProperty("Appearance"))
    {
        return false;
    }
    OpenSim::AbstractProperty& p = c.updPropertyByName("Appearance");
    auto* maybeAppearance = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&p);
    if (!maybeAppearance)
    {
        return false;
    }
    maybeAppearance->updValue().set_visible(v);
    return true;
}

glm::vec4 osc::GetSuggestedBoneColor() noexcept
{
    glm::vec4 usualDefault = {232.0f / 255.0f, 216.0f / 255.0f, 200.0f/255.0f, 1.0f};
    glm::vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
    float brightenAmount = 0.1f;
    return glm::mix(usualDefault, white, brightenAmount);
}