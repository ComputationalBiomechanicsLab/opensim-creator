#include "OpenSimHelpers.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/Perf.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <Simbody.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

static bool HasGreaterAlphaOrLowerMeshID(osc::ComponentDecoration const& a,
                                         osc::ComponentDecoration const& b)
{
    if (a.color.a != b.color.a)
    {
        // alpha descending, so non-opaque stuff is drawn last
        return a.color.a > b.color.a;
    }
    else
    {
        return a.mesh.get() < b.mesh.get();
    }
}

static osc::Transform TransformInGround(OpenSim::PhysicalFrame const& pf, SimTK::State const& st)
{
    return osc::ToTransform(pf.getTransformInGround(st));
}

// geometry rendering/handling support
namespace
{
    // generic handler for any `OpenSim::Component`
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
                                  float fixupScaleFactor,
                                  std::vector<osc::ComponentDecoration>& out)
    {
        glm::vec3 p1 = TransformPoint(TransformInGround(p2p.getBody1(), st), osc::ToVec3(p2p.getPoint1()));
        glm::vec3 p2 = TransformPoint(TransformInGround(p2p.getBody2(), st), osc::ToVec3(p2p.getPoint2()));

        float radius = 0.005f * fixupScaleFactor;
        osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, radius);

        out.emplace_back(
            osc::App::meshes().getCylinderMesh(),
            cylinderXform,
            glm::vec4{0.7f, 0.7f, 0.7f, 1.0f},
            &p2p
        );
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(OpenSim::Station const& s,
                       SimTK::State const& st,
                       float fixupScaleFactor,
                       std::vector<osc::ComponentDecoration>& out)
    {
        float radius = fixupScaleFactor * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        osc::Transform xform;
        xform.position = osc::ToVec3(s.getLocationInGround(st));
        xform.scale = {radius, radius, radius};

        out.emplace_back(
            osc::App::meshes().getSphereMesh(),
            xform,
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
            &s
        );
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(OpenSim::Body const& b,
                    SimTK::State const& st,
                    float fixupScaleFactor,
                    OpenSim::Component const*,
                    OpenSim::Component const* hovered,
                    std::vector<osc::ComponentDecoration>& out,
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
                osc::App::meshes().getSphereMesh(),
                t,
                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                &b
            );
        }

        HandleComponent(b, st, mdh, geomList, producer);
    }

    std::vector<glm::vec3> GetAllPathPoints(OpenSim::GeometryPath const& gp, SimTK::State const& st)
    {
        std::vector<glm::vec3> rv;

        OpenSim::Array<OpenSim::AbstractPathPoint*> const& pps = gp.getCurrentPath(st);

        for (int i = 0; i < pps.getSize(); ++i)
        {
            OpenSim::PathWrapPoint const* pwp = dynamic_cast<OpenSim::PathWrapPoint const*>(pps[i]);
            if (pwp)
            {
                osc::Transform body2ground = osc::ToTransform(pwp->getParentFrame().getTransformInGround(st));
                OpenSim::Array<SimTK::Vec3> const& wrapPath = const_cast<OpenSim::PathWrapPoint*>(pwp)->getWrapPath();

                for (int j = 0; j < wrapPath.getSize(); ++j)
                {
                    rv.push_back(body2ground * osc::ToVec3(wrapPath[j]));
                }
            }
            else
            {
                rv.push_back(osc::ToVec3(pps[i]->getLocationInGround(st)));
            }
        }

        return rv;
    }

    float GetSconeStyleAutomaticMuscleRadiusCalc(OpenSim::Muscle const& m)
    {
        float f = static_cast<float>(m.getMaxIsometricForce());
        float specificTension = 0.25e6f;  // SCONE magic number?
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

    glm::vec4 GetSconeStyleMuscleColor(OpenSim::Muscle const& musc, SimTK::State const& st, osc::MuscleColoringStyle s)
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

    void HandleMuscleSconeStyle(osc::CustomDecorationOptions const& opts,
                                OpenSim::Muscle const& muscle,
                                SimTK::State const& st,
                                float fixupScaleFactor,
                                OpenSim::ModelDisplayHints const& mdh,
                                std::vector<osc::ComponentDecoration>& out)
    {
        std::vector<glm::vec3> pps = GetAllPathPoints(muscle.getGeometryPath(), st);

        if (pps.empty())
        {
            // edge-case: there are no points in the muscle path
            return;
        }

        float const fiberUiRadius = GetMuscleSize(muscle, fixupScaleFactor, opts.getMuscleSizingStyle());
        float const tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        glm::vec4 const fiberColor = GetSconeStyleMuscleColor(muscle, st, opts.getMuscleColoringStyle());
        glm::vec4 const tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f, 1.0f};

        osc::ComponentDecoration fiberSpherePrototype =
        {
            osc::App::meshes().getSphereMesh(),
            osc::Transform{},
            fiberColor,
            &muscle
        };
        fiberSpherePrototype.transform.scale = {fiberUiRadius, fiberUiRadius, fiberUiRadius};

        osc::ComponentDecoration tendonSpherePrototype{fiberSpherePrototype};
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
                osc::App::meshes().getCylinderMesh(),
                cylinderXform,
                tendonColor,
                &muscle
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
                osc::App::meshes().getCylinderMesh(),
                cylinderXform,
                fiberColor,
                &muscle
            );
        };

        if (pps.size() == 1)
        {
            // edge-case: the muscle is a single point in space: just emit a sphere
            //
            // (this really should never happen, but you never know)
            emitFiberSphere(pps.front());
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
        glm::vec3 prevPos = pps.front();
        float prevTraversalPos = 0.0f;

        // draw first tendon
        if (prevTraversalPos < tendonLen)
        {
            // emit first tendon sphere
            emitTendonSphere(prevPos);
        }
        while (i < pps.size() && prevTraversalPos < tendonLen)
        {
            // emit remaining tendon cylinder + spheres

            glm::vec3 const& pos = pps[i];
            glm::vec3 prevToPos = pos - prevPos;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f)
            {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 tendonEnd = prevPos + scaler * prevToPos;

                emitTendonCylinder(prevPos, tendonEnd);
                emitTendonSphere(tendonEnd);

                prevPos = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else
            {
                emitTendonCylinder(prevPos, pos);
                emitTendonSphere(pos);

                i++;
                prevPos = pos;
                prevTraversalPos = traversalPos;
            }
        }

        // draw fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit first fiber sphere
            emitFiberSphere(prevPos);
        }
        while (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit remaining fiber cylinder + spheres

            glm::vec3 const& pos = pps[i];
            glm::vec3 prevToPos = pos - prevPos;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f)
            {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 fiberEndPos = prevPos + scaler * prevToPos;

                emitFiberCylinder(prevPos, fiberEndPos);
                emitFiberSphere(fiberEndPos);

                prevPos = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else
            {
                emitFiberCylinder(prevPos, pos);
                emitFiberSphere(pos);

                i++;
                prevPos = pos;
                prevTraversalPos = traversalPos;
            }
        }

        // draw second tendon
        if (i < pps.size())
        {
            // emit first tendon sphere
            emitTendonSphere(prevPos);
        }
        while (i < pps.size())
        {
            // emit remaining fiber cylinder + spheres

            glm::vec3 pos = pps[i];
            glm::vec3 prevToPos = pos - prevPos;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;

            emitTendonCylinder(prevPos, pos);
            emitTendonSphere(pos);

            i++;
            prevPos = pos;
            prevTraversalPos = traversalPos;
        }
    }

    void HandleMuscleOpenSimStyle(osc::CustomDecorationOptions const& opts,
                                  OpenSim::Muscle const& musc,
                                  SimTK::State const& st,
                                  float fixupScaleFactor,
                                  OpenSim::Component const** currentComponent,
                                  OpenSim::ModelDisplayHints const& mdh,
                                  SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                                  osc::DecorativeGeometryHandler& producer,
                                  std::vector<osc::ComponentDecoration>& out)
    {
        std::vector<glm::vec3> pps = GetAllPathPoints(musc.getGeometryPath(), st);

        if (pps.empty())
        {
            return;
        }

        float const fiberUiRadius = GetMuscleSize(musc, fixupScaleFactor, opts.getMuscleSizingStyle());
        glm::vec4 const fiberColor = GetSconeStyleMuscleColor(musc, st, opts.getMuscleColoringStyle());

        auto emitSphere = [&](glm::vec3 const& pos)
        {
            osc::Transform t;
            t.scale *= fiberUiRadius;
            t.position = pos;

            out.emplace_back(
                osc::App::meshes().getSphereMesh(),
                t,
                fiberColor,
                &musc
            );
        };

        auto emitCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            out.emplace_back(
                osc::App::meshes().getCylinderMesh(),
                cylinderXform,
                fiberColor,
                &musc
            );
        };

        if (mdh.get_show_path_points())
        {
            emitSphere(pps.front());
        }

        for (std::size_t i = 1; i < pps.size(); ++i)
        {
            emitCylinder(pps[i - 1], pps[i]);

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
                            float fixupScaleFactor,
                            OpenSim::Component const** currentComponent,
                            OpenSim::ModelDisplayHints const& mdh,
                            SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                            osc::DecorativeGeometryHandler& producer,
                            std::vector<osc::ComponentDecoration>& out)
    {
        if (gp.hasOwner())
        {
            if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&gp.getOwner()); musc)
            {
                // owner is a muscle, coerce selection "hit" to the muscle
                *currentComponent = musc;

                if (opts.getMuscleDecorationStyle() == osc::MuscleDecorationStyle::Scone)
                {
                    HandleMuscleSconeStyle(opts, *musc, st, fixupScaleFactor, mdh, out);
                    return;  // don't let it fall through to the generic handler
                }
                else
                {
                    HandleMuscleOpenSimStyle(opts, *musc, st, fixupScaleFactor, currentComponent, mdh, geomList, producer, out);
                    return;
                }
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

    // used whenever the SimTK backend emits something
    class OpenSimDecorationConsumer final : public osc::DecorationConsumer {
    public:
        OpenSimDecorationConsumer(std::vector<osc::ComponentDecoration>* out,
                                  OpenSim::Component const** currentComponent) :
            m_Out{std::move(out)},
            m_CurrentComponent{std::move(currentComponent)}
        {
        }

        void operator()(std::shared_ptr<osc::Mesh> const& mesh, osc::Transform const& t, glm::vec4 const& color) override
        {
            m_Out->emplace_back(mesh, t, color, *m_CurrentComponent);
        }

    private:
        std::vector<osc::ComponentDecoration>* m_Out;
        OpenSim::Component const** m_CurrentComponent;
    };

    void GenerateDecorationEls(osc::CustomDecorationOptions const& opts,
                               OpenSim::Model const& m,
                               SimTK::State const& st,
                               float fixupScaleFactor,
                               std::vector<osc::ComponentDecoration>& out,
                               OpenSim::Component const* selected,
                               OpenSim::Component const* hovered)
    {
        out.clear();

        OpenSim::Component const* currentComponent = nullptr;

        OpenSimDecorationConsumer consumer{&out, &currentComponent};

        osc::MeshCache& meshCache = osc::App::meshes();

        osc::DecorativeGeometryHandler producer{
            meshCache,
            m.getSystem().getMatterSubsystem(),
            st,
            fixupScaleFactor,
            consumer
        };

        OpenSim::ModelDisplayHints const& mdh = m.getDisplayHints();
        SimTK::Array_<SimTK::DecorativeGeometry> geomList;

        for (OpenSim::Component const& c : m.getComponentList())
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
                HandlePointToPointSpring(*p2p, st, fixupScaleFactor, out);
            }
            else if (typeid(c) == typeid(OpenSim::Station))
            {
                HandleStation(static_cast<OpenSim::Station const&>(c), st, fixupScaleFactor, out);
            }
            else if (auto const* body = dynamic_cast<OpenSim::Body const*>(&c))
            {
                HandleBody(*body, st, fixupScaleFactor, selected, hovered, out, mdh, geomList, producer);
            }
            else if (auto const* gp = dynamic_cast<OpenSim::GeometryPath const*>(&c))
            {
                HandleGeometryPath(opts, *gp, st, fixupScaleFactor, &currentComponent, mdh, geomList, producer, out);
            }
            else
            {
                HandleComponent(c, st, mdh, geomList, producer);
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

OpenSim::AbstractOutput const* osc::FindOutput(OpenSim::Component const& c, std::string const& outputName)
{
    OpenSim::AbstractOutput const* rv = nullptr;
    try
    {
        rv = &c.getOutput(outputName);
    }
    catch (...)
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
    if (!c.hasOwner())
    {
        log::error("cannot delete %s: it has no owner", c.getName().c_str());
        return false;
    }

    if (&c.getRoot() != &m)
    {
        log::error("cannot delete %s: it is not owned by the provided model");
        return false;
    }

    OpenSim::Component& owner = const_cast<OpenSim::Component&>(c.getOwner());

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

    if (auto* js = dynamic_cast<OpenSim::JointSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*js, dynamic_cast<OpenSim::Joint*>(&c));
    }
    else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*bs, dynamic_cast<OpenSim::Body*>(&c));
    }
    else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*wos, dynamic_cast<OpenSim::WrapObject*>(&c));
    }
    else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*cs, dynamic_cast<OpenSim::Controller*>(&c));
    }
    else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*conss, dynamic_cast<OpenSim::Constraint*>(&c));
    }
    else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*fs, dynamic_cast<OpenSim::Force*>(&c));
    }
    else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*ms, dynamic_cast<OpenSim::Marker*>(&c));
    }
    else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(&owner); cgs)
    {
        rv = TryDeleteItemFromSet(*cgs, dynamic_cast<OpenSim::ContactGeometry*>(&c));
    }
    else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(&owner))
    {
        rv = TryDeleteItemFromSet(*ps, dynamic_cast<OpenSim::Probe*>(&c));
    }
    else if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(&owner))
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
    else if (auto const* geom = FindAncestorWithType<OpenSim::Geometry>(&c))
    {
        // delete an OpenSim::Geometry from its owning OpenSim::Frame

        if (auto const* frame = FindAncestorWithType<OpenSim::Frame>(geom))
        {
            // its owner is a frame, which holds the geometry in a list property

            // make a copy of the property containing the geometry and
            // only copy over the not-deleted geometry into the copy
            //
            // this is necessary because OpenSim::Property doesn't seem
            // to support list element deletion, but does support full
            // assignment

            auto& mframe = const_cast<OpenSim::Frame&>(*frame);
            OpenSim::ObjectProperty<OpenSim::Geometry>& prop =
                static_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(mframe.updProperty_attached_geometry());

            std::unique_ptr<OpenSim::ObjectProperty<OpenSim::Geometry>> copy{prop.clone()};
            copy->clear();
            for (int i = 0; i < prop.size(); ++i) {
                OpenSim::Geometry& g = prop[i];
                if (&g != geom) {
                    copy->adoptAndAppendValue(g.clone());
                }
            }

            prop.assign(*copy);

            rv = true;
        }
    }

    return rv;
}

void osc::GenerateModelDecorations(VirtualConstModelStatePair const& p,
                                   std::vector<ComponentDecoration>& out,
                                   CustomDecorationOptions opts)
{
    out.clear();

    {
        OSC_PERF("scene generation");
        GenerateDecorationEls(opts, p.getModel(), p.getState(), p.getFixupScaleFactor(), out, p.getSelected(), p.getHovered());
    }

    {
        OSC_PERF("scene sorting");
        Sort(out, HasGreaterAlphaOrLowerMeshID);
    }
}

void osc::UpdateSceneBVH(nonstd::span<ComponentDecoration const> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());

    for (auto const& el : sceneEls)
    {
        aabbs.push_back(GetWorldspaceAABB(el));
    }

    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
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
    std::vector<ComponentDecoration> ses;
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
    auto model = std::make_unique<OpenSim::Model>(p.string());
    return std::make_unique<osc::UndoableModelStatePair>(std::move(model));
}

SimTK::State& osc::Initialize(OpenSim::Model& model)
{
    OSC_PERF("model update");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)
    model.clearConnections();        // clears any potentially stale pointers that can be retained by OpenSim::Socket<T> (see #263)
    model.buildSystem();             // creates a new underlying physics system
    return model.initializeState();  // creates+returns a new working state
}

// returns -1 if joint isn't in a set or cannot be found
int osc::FindJointInParentJointSet(OpenSim::Joint const& joint)
{
    auto const* parentJointset =
        joint.hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&joint.getOwner()) : nullptr;

    if (!parentJointset)
    {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return -1;
    }

    OpenSim::JointSet const& js = *parentJointset;

    int idx = -1;
    for (int i = 0; i < js.getSize(); ++i) {
        OpenSim::Joint const* j = &js[i];
        if (j == &joint) {
            idx = i;
            break;
        }
    }

    return idx;
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