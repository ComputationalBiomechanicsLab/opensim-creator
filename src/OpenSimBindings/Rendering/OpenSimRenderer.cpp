#include "OpenSimRenderer.hpp"

#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Transform.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/SimTKDecorationConsumer.hpp"
#include "src/OpenSimBindings/Rendering/SimTKRenderer.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
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
#include <string>
#include <utility>
#include <vector>

// compute lines of action
namespace
{
    // (a memory-safe stdlib version of OpenSim::GeometryPath::getPointForceDirections)
    std::vector<std::unique_ptr<OpenSim::PointForceDirection>> GetPointForceDirections(
        OpenSim::GeometryPath const& path,
        SimTK::State const& st)
    {
        OpenSim::Array<OpenSim::PointForceDirection*> pfds;
        path.getPointForceDirections(st, &pfds);

        std::vector<std::unique_ptr<OpenSim::PointForceDirection>> rv;
        rv.reserve(pfds.getSize());
        for (int i = 0; i < pfds.getSize(); ++i)
        {
            rv.emplace_back(pfds[i]);
        }
        return rv;
    }

    // returns the "effective" origin point of a muscle PFD sequence
    ptrdiff_t GetEffectiveOrigin(std::vector<std::unique_ptr<OpenSim::PointForceDirection>> const& pfds)
    {
        OSC_ASSERT_ALWAYS(!pfds.empty());

        // move forward through the PFD sequence until a different frame is found
        //
        // the PFD before that one is the effective origin
        auto const it = std::find_if(
            pfds.begin() + 1,
            pfds.end(),
            [&first = pfds.front()](auto const& pfd) { return &pfd->frame() != &first->frame(); }
        );
        return std::distance(pfds.begin(), it) - 1;
    }

    ptrdiff_t GetEffectiveInsertion(std::vector<std::unique_ptr<OpenSim::PointForceDirection>> const& pfds)
    {
        OSC_ASSERT_ALWAYS(!pfds.empty());

        // move backward through the PFD sequence until a different frame is found
        //
        // the PFD after that one is the effective insertion
        auto const rit = std::find_if(
            pfds.rbegin() + 1,
            pfds.rend(),
            [&last = pfds.back()](auto const& pfd) { return &pfd->frame() != &last->frame(); }
        );
        return std::distance(pfds.begin(), rit.base());
    }

    // returns an index range into the provided array that contains only
    // effective attachment points? (see: https://github.com/modenaxe/MuscleForceDirection/blob/master/CPP/MuscleForceDirection/MuscleForceDirection.cpp)
    std::pair<ptrdiff_t, ptrdiff_t> GetEffectiveAttachmentIndices(std::vector<std::unique_ptr<OpenSim::PointForceDirection>> const& pfds)
    {
        return {GetEffectiveOrigin(pfds), GetEffectiveInsertion(pfds)};
    }

    std::pair<ptrdiff_t, ptrdiff_t> GetAnatomicalAttachmentIndices(std::vector<std::unique_ptr<OpenSim::PointForceDirection>> const& pfds)
    {
        OSC_ASSERT(!pfds.empty());

        return {0, pfds.size() - 1};
    }

    glm::vec3 GetLocationInGround(OpenSim::PointForceDirection& pf, SimTK::State const& st)
    {
        SimTK::Vec3 const location = pf.frame().findStationLocationInGround(st, pf.point());
        return osc::ToVec3(location);
    }

    struct LinesOfActionConfig final {

        // as opposed to using "anatomical"
        bool useEffectiveInsertion = true;
    };

    struct LinesOfAction final {
        glm::vec3 originPos;
        glm::vec3 originDirection;
        glm::vec3 insertionPos;
        glm::vec3 insertionDirection;
    };

    std::optional<LinesOfAction> TryGetLinesOfAction(
        OpenSim::Muscle const& muscle,
        SimTK::State const& st,
        LinesOfActionConfig const& config)
    {
        std::vector<std::unique_ptr<OpenSim::PointForceDirection>> const pfds = GetPointForceDirections(muscle.getGeometryPath(), st);
        if (pfds.size() < 2)
        {
            return std::nullopt;  // not enough PFDs to compute a line of action
        }

        std::pair<ptrdiff_t, ptrdiff_t> const attachmentIndexRange = config.useEffectiveInsertion ?
            GetEffectiveAttachmentIndices(pfds) :
            GetAnatomicalAttachmentIndices(pfds);

        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.first && attachmentIndexRange.first < osc::ssize(pfds));
        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.second && attachmentIndexRange.second < osc::ssize(pfds));

        if (attachmentIndexRange.first >= attachmentIndexRange.second)
        {
            return std::nullopt;  // not enough *unique* PFDs to compute a line of action
        }

        glm::vec3 const originPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first), st);
        glm::vec3 const pointAfterOriginPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first + 1), st);
        glm::vec3 const originDir = glm::normalize(pointAfterOriginPos - originPos);

        glm::vec3 const insertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second), st);
        glm::vec3 const pointAfterInsertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second - 1), st);
        glm::vec3 const insertionDir = glm::normalize(pointAfterInsertionPos - insertionPos);

        return LinesOfAction{originPos, originDir, insertionPos, insertionDir};
    }
}

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

    // helper: convert a physical frame's transform to ground into an osc::Transform
    osc::Transform TransformInGround(
        OpenSim::PhysicalFrame const& pf,
        SimTK::State const& st)
    {
        return osc::ToTransform(pf.getTransformInGround(st));
    }

    // helper struct: simplification of a point in a geometry path
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
    glm::vec4 GetMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
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

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(OpenSim::Muscle const& musc, float fixupScaleFactor, osc::MuscleSizingStyle s)
    {
        switch (s) {
        case osc::MuscleSizingStyle::PcsaDerived:
            return GetSconeStyleAutomaticMuscleRadiusCalc(musc) * fixupScaleFactor;
        case osc::MuscleSizingStyle::OpenSim:
        default:
            return 0.005f * fixupScaleFactor;
        }
    }
}

// geometry handlers
namespace
{
    // generic decoration handler for any `OpenSim::Component`
    void HandleComponent(
        OpenSim::Component const& c,
        SimTK::State const& st,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
        osc::SimTKRenderer& handler)
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
    void HandlePointToPointSpring(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::PointToPointSpring const& p2p,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
    {
        if (!opts.getShouldShowPointToPointSprings())
        {
            return;
        }

        glm::vec3 p1 = TransformInGround(p2p.getBody1(), st) * osc::ToVec3(p2p.getPoint1());
        glm::vec3 p2 = TransformInGround(p2p.getBody2(), st) * osc::ToVec3(p2p.getPoint2());

        float radius = 0.005f * fixupScaleFactor;
        osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, radius);

        out(p2p, osc::SceneDecoration
        {
            meshCache.getCylinderMesh(),
            cylinderXform,
            glm::vec4{0.7f, 0.7f, 0.7f, 1.0f},
            p2p.getAbsolutePathString(),
            ComputeFlags(p2p, selected, hovered)
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(
        osc::MeshCache& meshCache,
        OpenSim::Station const& s,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
    {
        float radius = fixupScaleFactor * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        osc::Transform xform;
        xform.position = osc::ToVec3(s.getLocationInGround(st));
        xform.scale = {radius, radius, radius};

        out(s, osc::SceneDecoration
        {
            meshCache.getSphereMesh(),
            xform,
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
            s.getAbsolutePathString(),
            ComputeFlags(s, selected, hovered)
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void HandleScapulothoracicJoint(
        osc::MeshCache& meshCache,
        OpenSim::ScapulothoracicJoint const& j,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
    {
        osc::Transform t = osc::ToTransform(j.getParentFrame().getTransformInGround(st));
        t.scale = osc::ToVec3(j.get_thoracic_ellipsoid_radii_x_y_z());

        out(j, osc::SceneDecoration
        {
            meshCache.getSphereMesh(),
            t,
            glm::vec4{1.0f, 1.0f, 0.0f, 0.2f},
            j.getAbsolutePathString(),
            ComputeFlags(j, selected, hovered)
        });
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::Body const& b,
        SimTK::State const& st,
        float fixupScaleFactor,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
        osc::SimTKRenderer& producer)
    {
        // bodies are drawn normally but *also* draw a center-of-mass sphere if they are
        // currently hovered
        if (opts.getShouldShowCentersOfMass() && b.getMassCenter() != SimTK::Vec3{0.0, 0.0, 0.0})
        {
            float radius = fixupScaleFactor * 0.005f;
            osc::Transform t = TransformInGround(b, st);
            t.position = osc::TransformPoint(t, osc::ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            out(b, osc::SceneDecoration
            {
                meshCache.getSphereMesh(),
                t,
                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                b.getAbsolutePathString(),
                ComputeFlags(b, selected, hovered)
            });
        }

        HandleComponent(b, st, mdh, geomList, producer);
    }

    // OSC-specific decoration handler for `OpenSim::Muscle` ("SCONE"-style: i.e. tendons + muscle)
    void HandleMuscleFibersAndTendons(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::Muscle const& muscle,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        OpenSim::ModelDisplayHints const&,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
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
            meshCache.getSphereMesh(),
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
            osc::SceneDecoration copy{tendonSpherePrototype};
            copy.transform.position = pos;
            out(muscle, std::move(copy));
        };
        auto emitTendonCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, tendonUiRadius);

            out(muscle, osc::SceneDecoration
            {
                meshCache.getCylinderMesh(),
                cylinderXform,
                tendonColor,
                muscleAbsPath,
                flags
            });
        };
        auto emitFiberSphere = [&](glm::vec3 const& pos)
        {
            osc::SceneDecoration copy{fiberSpherePrototype};
            copy.transform.position = pos;
            out(muscle, std::move(copy));
        };
        auto emitFiberCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            out(muscle, osc::SceneDecoration
            {
                meshCache.getCylinderMesh(),
                cylinderXform,
                fiberColor,
                muscleAbsPath,
                flags
            });
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
    void HandleMuscleOpenSimStyle(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        OpenSim::Component const**,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>&,
        osc::SimTKRenderer&,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
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

            out(musc, osc::SceneDecoration
            {
                meshCache.getSphereMesh(),
                t,
                fiberColor,
                pp.maybePathPoint ? pp.maybePathPoint->getAbsolutePathString() : absPath,
                sphereFlags
            });
        };

        auto emitCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            out(musc, osc::SceneDecoration
            {
                meshCache.getCylinderMesh(),
                cylinderXform,
                fiberColor,
                absPath,
                flags
            });
        };

        if (mdh.get_show_path_points())
        {
            emitSphere(pps.front());
        }
        for (size_t i = 1; i < pps.size(); ++i)
        {
            emitCylinder(pps[i - 1].location, pps[i].location);

            if (mdh.get_show_path_points())
            {
                emitSphere(pps[i]);
            }
        }
    }

    void HandleLinesOfAction(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
    {
        // if options request, render effective muscle lines of action
        if (opts.getShouldShowEffectiveMuscleLinesOfAction())
        {
            // render lines of action (todo: should be behind a UI toggle for on vs. effective vs. anatomical etc.)
            LinesOfActionConfig config{};
            config.useEffectiveInsertion = true;
            if (std::optional<LinesOfAction> loas = TryGetLinesOfAction(musc, st, config))
            {
                // origin arrow
                {
                    osc::ArrowProperties p;
                    p.worldspaceStart = loas->originPos;
                    p.worldspaceEnd = loas->originPos + (fixupScaleFactor*0.1f)*loas->originDirection;
                    p.tipLength = (fixupScaleFactor*0.015f);
                    p.headThickness = (fixupScaleFactor*0.01f);
                    p.neckThickness = (fixupScaleFactor*0.006f);
                    p.color = {0.0f, 1.0f, 0.0f, 1.0f};

                    osc::DrawArrow(meshCache, p, [&musc, &out, selected, hovered](osc::SceneDecoration&& d)
                    {
                        d.id = musc.getAbsolutePathString();
                        d.flags = ComputeFlags(musc, selected, hovered);
                        out(musc, std::move(d));
                    });
                }

                // insertion arrow
                {
                    osc::ArrowProperties p;
                    p.worldspaceStart = loas->insertionPos;
                    p.worldspaceEnd = loas->insertionPos + (fixupScaleFactor*0.1f)*loas->insertionDirection;
                    p.tipLength = (fixupScaleFactor*0.015f);
                    p.headThickness = (fixupScaleFactor*0.01f);
                    p.neckThickness = (fixupScaleFactor*0.006f);
                    p.color = {0.0f, 1.0f, 0.0f, 1.0f};

                    osc::DrawArrow(meshCache, p, [&musc, &out, selected, hovered](osc::SceneDecoration&& d)
                    {
                        d.id = musc.getAbsolutePathString();
                        d.flags = ComputeFlags(musc, selected, hovered);
                        out(musc, std::move(d));
                    });
                }
            }
        }

        // if options request, render anatomical muscle lines of action
        if (opts.getShouldShowAnatomicalMuscleLinesOfAction())
        {
            // render lines of action (todo: should be behind a UI toggle for on vs. effective vs. anatomical etc.)
            LinesOfActionConfig config{};
            config.useEffectiveInsertion = false;
            if (std::optional<LinesOfAction> loas = TryGetLinesOfAction(musc, st, config))
            {
                // origin arrow
                {
                    osc::ArrowProperties p;
                    p.worldspaceStart = loas->originPos;
                    p.worldspaceEnd = loas->originPos + (fixupScaleFactor*0.1f)*loas->originDirection;
                    p.tipLength = (fixupScaleFactor*0.015f);
                    p.headThickness = (fixupScaleFactor*0.01f);
                    p.neckThickness = (fixupScaleFactor*0.006f);
                    p.color = {1.0f, 0.0f, 0.0f, 1.0f};

                    osc::DrawArrow(meshCache, p, [&musc, &out, selected, hovered](osc::SceneDecoration&& d)
                    {
                        d.id = musc.getAbsolutePathString();
                        d.flags = ComputeFlags(musc, selected, hovered);
                        out(musc, std::move(d));
                    });
                }

                // insertion arrow
                {
                    osc::ArrowProperties p;
                    p.worldspaceStart = loas->insertionPos;
                    p.worldspaceEnd = loas->insertionPos + (fixupScaleFactor*0.1f)*loas->insertionDirection;
                    p.tipLength = (fixupScaleFactor*0.015f);
                    p.headThickness = (fixupScaleFactor*0.01f);
                    p.neckThickness = (fixupScaleFactor*0.006f);
                    p.color = {1.0f, 0.0f, 0.0f, 1.0f};

                    osc::DrawArrow(meshCache, p, [&musc, &out, selected, hovered](osc::SceneDecoration&& d)
                    {
                        d.id = musc.getAbsolutePathString();
                        d.flags = ComputeFlags(musc, selected, hovered);
                        out(musc, std::move(d));
                    });
                }
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(
        osc::MeshCache& meshCache,
        osc::CustomDecorationOptions const& opts,
        OpenSim::GeometryPath const& gp,
        SimTK::State const& st,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        float fixupScaleFactor,
        OpenSim::Component const** currentComponent,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
        osc::SimTKRenderer& producer,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
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

                HandleLinesOfAction(
                    meshCache,
                    opts,
                    *musc,
                    st,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    out
                );

                switch (opts.getMuscleDecorationStyle())
                {
                case osc::MuscleDecorationStyle::FibersAndTendons:
                    HandleMuscleFibersAndTendons(
                        meshCache,
                        opts,
                        *musc,
                        st,
                        selected,
                        hovered,
                        fixupScaleFactor,
                        mdh,
                        out
                    );
                    return;
                case osc::MuscleDecorationStyle::Hidden:
                    return;  // just don't generate them
                case osc::MuscleDecorationStyle::OpenSim:
                default:
                    HandleMuscleOpenSimStyle(
                        meshCache,
                        opts,
                        *musc,
                        st,
                        selected,
                        hovered,
                        fixupScaleFactor,
                        currentComponent,
                        mdh,
                        geomList,
                        producer,
                        out
                    );
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
        osc::CustomDecorationOptions const&,
        OpenSim::FrameGeometry const& frameGeometry,
        SimTK::State const& st,
        OpenSim::Component const*,
        OpenSim::Component const*,
        float,
        OpenSim::Component const** currentComponent,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
        osc::SimTKRenderer& producer,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
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
    class OpenSimDecorationConsumer2 final : public osc::SimTKDecorationConsumer {
    public:
        OpenSimDecorationConsumer2(
            osc::VirtualConstModelStatePair const* msp,
            std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const* out,
            OpenSim::Component const** currentComponent) :
            m_Msp{std::move(msp)},
            m_Out{out},
            m_CurrentComponent{std::move(currentComponent)}
        {
        }

        void operator()(osc::Mesh const& mesh, osc::Transform const& t, glm::vec4 const& color) override
        {
            std::string absPath = (*m_CurrentComponent)->getAbsolutePathString();
            osc::SceneDecoration decoration{mesh, t, color, std::move(absPath), ComputeFlags(**m_CurrentComponent, m_Selected, m_Hovered)};
            (*m_Out)(**m_CurrentComponent, std::move(decoration));
        }

    private:
        osc::VirtualConstModelStatePair const* m_Msp;
        OpenSim::Component const* m_Selected = m_Msp->getSelected();
        OpenSim::Component const* m_Hovered = m_Msp->getHovered();
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const* m_Out;
        OpenSim::Component const** m_CurrentComponent;
    };

    // generates a sequence of OSC decoration from an OpenSim model + state
    void GenerateDecorationEls(
        osc::MeshCache& meshCache,
        osc::VirtualConstModelStatePair const& msp,
        osc::CustomDecorationOptions const& opts,
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out)
    {
        // assumed to be valid during the decoration generation
        OpenSim::Model const& model = msp.getModel();
        SimTK::State const& state = msp.getState();
        OpenSim::Component const* selected = msp.getSelected();
        OpenSim::Component const* hovered = msp.getHovered();
        float fixupScaleFactor = msp.getFixupScaleFactor();
        OpenSim::ModelDisplayHints const& mdh = msp.getModel().getDisplayHints();

        // gets called whenever OpenSim is emitting stuff via `generateDecorations`
        OpenSim::Component const* currentComponent = nullptr;
        OpenSimDecorationConsumer2 consumer{&msp, &out, &currentComponent};

        // generates mesh/object instances for each SimTK::DecorativeGeometry it's given
        osc::SimTKRenderer producer
        {
            meshCache,
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
                HandlePointToPointSpring(
                    meshCache,
                    opts,
                    *p2p,
                    state,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    out
                );
            }
            else if (typeid(c) == typeid(OpenSim::Station))  // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            {
                HandleStation(
                    meshCache,
                    static_cast<OpenSim::Station const&>(c),
                    state,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    out
                );
            }
            else if (auto const* scapulo = dynamic_cast<OpenSim::ScapulothoracicJoint const*>(&c); scapulo && opts.getShouldShowScapulo())
            {
                HandleScapulothoracicJoint(
                    meshCache,
                    *scapulo,
                    state,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    out
                );
            }
            else if (auto const* body = dynamic_cast<OpenSim::Body const*>(&c))
            {
                HandleBody(
                    meshCache,
                    opts,
                    *body,
                    state,
                    fixupScaleFactor,
                    selected,
                    hovered,
                    out,
                    mdh,
                    geomList,
                    producer
                );
            }
            else if (auto const* gp = dynamic_cast<OpenSim::GeometryPath const*>(&c))
            {
                HandleGeometryPath(
                    meshCache,
                    opts,
                    *gp,
                    state,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    &currentComponent,
                    mdh,
                    geomList,
                    producer,
                    out
                );
            }
            else if (auto const* fg = dynamic_cast<OpenSim::FrameGeometry const*>(&c))
            {
                HandleFrameGeometry(
                    opts,
                    *fg,
                    state,
                    selected,
                    hovered,
                    fixupScaleFactor,
                    &currentComponent,
                    mdh,
                    geomList,
                    producer,
                    out
                );
            }
            else
            {
                // generic handler
                HandleComponent(
                    c,
                    state,
                    mdh,
                    geomList,
                    producer
                );
            }
        }
    }
}

void osc::GenerateModelDecorations(
    MeshCache& meshCache,
    VirtualConstModelStatePair const& modelState,
    CustomDecorationOptions const& opts,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
{
    OSC_PERF("scene generation");

    GenerateDecorationEls(
        meshCache,
        modelState,
        opts,
        out
    );
}

float osc::GetRecommendedScaleFactor(
    MeshCache& meshCache,
    VirtualConstModelStatePair const& p,
    CustomDecorationOptions const& options)
{
    // generate decorations as if they were empty-sized and union their
    // AABBs to get an idea of what the "true" scale of the model probably
    // is (without the model containing oversized frames, etc.)
    std::vector<SceneDecoration> decs;
    GenerateModelDecorations(
        meshCache,
        p,
        options,
        [&decs](OpenSim::Component const&, SceneDecoration&& dec)
        {
            decs.push_back(std::move(dec));
        }
    );

    if (decs.empty())
    {
        return 1.0f;
    }

    AABB aabb = GetWorldspaceAABB(decs[0]);
    for (SceneDecoration const& dec : decs)
    {
        aabb = Union(aabb, GetWorldspaceAABB(dec));
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