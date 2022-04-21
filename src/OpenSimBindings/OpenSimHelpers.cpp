#include "OpenSimHelpers.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
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
        float radius = fixupScaleFactor * 0.005f;

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

    void HandleSconeMusclePath(osc::CustomDecorationOptions const& opts,
                              OpenSim::Muscle const& muscle,
                              SimTK::State const& st,
                              OpenSim::Component const** currentComponent,
                              OpenSim::ModelDisplayHints const& mdh,
                              SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                              osc::DecorativeGeometryHandler& producer)
    {
        OpenSim::PathPointSet const& pps = muscle.getGeometryPath().getPathPointSet();
        if (pps.getSize() == 0)
        {
            return;  // no points in the path
        }

        double mLen = muscle.getFiberLength(st);
        mLen = std::clamp(mLen, 0.0, mLen);

        double tLen = muscle.getTendonLength(st);
        tLen = std::clamp(tLen, 0.0, tLen);


        // add first point
        pps[0].getLocationInGround(st);

        // TODO: not-generic handling
        HandleComponent(muscle.getGeometryPath(), st, mdh, geomList, producer);
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(osc::CustomDecorationOptions const& opts,
                            OpenSim::GeometryPath const& gp,
                            SimTK::State const& st,
                            OpenSim::Component const** currentComponent,
                            OpenSim::ModelDisplayHints const& mdh,
                            SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                            osc::DecorativeGeometryHandler& producer)
    {
        if (gp.hasOwner())
        {
            if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&gp.getOwner()); musc)
            {
                // owner is a muscle, coerce selection "hit" to the muscle
                *currentComponent = musc;

                if (opts.getMuscleDecorationStyle() == osc::MuscleDecorationStyle::Scone)
                {
                    HandleSconeMusclePath(opts, *musc, st, currentComponent, mdh, geomList, producer);
                    return;  // don't let it fall through to the generic handler
                }
            }
        }

        // if it falls through this far, just handle the muscle generically
        HandleComponent(gp, st, mdh, geomList, producer);
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
                HandleGeometryPath(opts, *gp, st, &currentComponent, mdh, geomList, producer);
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

std::vector<OpenSim::AbstractSocket const*> osc::GetAllSockets(OpenSim::Component& c)
{
    std::vector<OpenSim::AbstractSocket const*> rv;

    for (std::string const& name : c.getSocketNames())
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        rv.push_back(&sock);
    }

    return rv;
}

std::vector<OpenSim::AbstractSocket const*> osc::GetSocketsWithTypeName(OpenSim::Component& c, std::string_view typeName)
{
    std::vector<OpenSim::AbstractSocket const*> rv;

    for (std::string const& name : c.getSocketNames())
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        if (sock.getConnecteeTypeName() == typeName)
        {
            rv.push_back(&sock);
        }
    }

    return rv;
}

std::vector<OpenSim::AbstractSocket const*> osc::GetPhysicalFrameSockets(OpenSim::Component& c)
{
    return GetSocketsWithTypeName(c, "PhysicalFrame");
}

bool osc::IsConnectedViaSocketTo(OpenSim::Component& c, OpenSim::Component const& other)
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

bool osc::IsAnyComponentConnectedViaSocketTo(OpenSim::Component& root, OpenSim::Component const& other)
{
    if (IsConnectedViaSocketTo(root, other))
    {
        return true;
    }

    for (OpenSim::Component& c : root.updComponentList())
    {
        if (IsConnectedViaSocketTo(c, other))
        {
            return true;
        }
    }

    return false;
}

std::vector<OpenSim::Component*> osc::GetAnyComponentsConnectedViaSocketTo(OpenSim::Component& root, OpenSim::Component const& other)
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

OpenSim::Component const* osc::FindComponent(OpenSim::Component const& c, OpenSim::ComponentPath const& cp)
{
    static OpenSim::ComponentPath const g_EmptyPath;

    if (cp == g_EmptyPath)
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

bool osc::ContainsComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
{
    return FindComponent(root, cp);
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

    if (auto* js = dynamic_cast<OpenSim::JointSet*>(&owner))
    {
        return TryDeleteItemFromSet(*js, dynamic_cast<OpenSim::Joint*>(&c));
    }
    else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(&owner))
    {
        return TryDeleteItemFromSet(*bs, dynamic_cast<OpenSim::Body*>(&c));
    }
    else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(&owner))
    {
        return TryDeleteItemFromSet(*wos, dynamic_cast<OpenSim::WrapObject*>(&c));
    }
    else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(&owner))
    {
        return TryDeleteItemFromSet(*cs, dynamic_cast<OpenSim::Controller*>(&c));
    }
    else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(&owner))
    {
        return TryDeleteItemFromSet(*conss, dynamic_cast<OpenSim::Constraint*>(&c));
    }
    else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(&owner))
    {
        return TryDeleteItemFromSet(*fs, dynamic_cast<OpenSim::Force*>(&c));
    }
    else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(&owner))
    {
        return TryDeleteItemFromSet(*ms, dynamic_cast<OpenSim::Marker*>(&c));
    }
    else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(&owner); cgs)
    {
        return TryDeleteItemFromSet(*cgs, dynamic_cast<OpenSim::ContactGeometry*>(&c));
    }
    else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(&owner))
    {
        return TryDeleteItemFromSet(*ps, dynamic_cast<OpenSim::Probe*>(&c));
    }
    else if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(&owner))
    {
        if (auto* app = dynamic_cast<OpenSim::AbstractPathPoint*>(&c))
        {
            return TryDeleteItemFromSet(gp->updPathPointSet(), app);
        }
        else if (auto* pw = dynamic_cast<OpenSim::PathWrap*>(&c))
        {
            return TryDeleteItemFromSet(gp->updWrapSet(), pw);
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

            return true;
        }
    }
    return false;
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
        aabbs.push_back(el.worldspaceAABB);
    }

    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
}

void osc::CopyCommonJointProperties(OpenSim::Joint const& src, OpenSim::Joint& dest)
{
    dest.setName(src.getName());

    // copy owned frames
    dest.updProperty_frames().assign(src.getProperty_frames());

    // copy, or reference, the parent based on whether the source owns it
    {
        OpenSim::PhysicalFrame const& srcParent = src.getParentFrame();
        bool parentAssigned = false;
        for (int i = 0; i < src.getProperty_frames().size(); ++i) {
            if (&src.get_frames(i) == &srcParent) {
                // the source's parent is also owned by the source, so we need to
                // ensure the destination refers to its own (cloned, above) copy
                dest.connectSocket_parent_frame(dest.get_frames(i));
                parentAssigned = true;
                break;
            }
        }
        if (!parentAssigned) {
            // the source's parent is a reference to some frame that the source
            // doesn't, itself, own, so the destination should just also refer
            // to the same (not-owned) frame
            dest.connectSocket_parent_frame(srcParent);
        }
    }

    // copy, or reference, the child based on whether the source owns it
    {
        OpenSim::PhysicalFrame const& srcChild = src.getChildFrame();
        bool childAssigned = false;
        for (int i = 0; i < src.getProperty_frames().size(); ++i) {
            if (&src.get_frames(i) == &srcChild) {
                // the source's child is also owned by the source, so we need to
                // ensure the destination refers to its own (cloned, above) copy
                dest.connectSocket_child_frame(dest.get_frames(i));
                childAssigned = true;
                break;
            }
        }
        if (!childAssigned) {
            // the source's child is a reference to some frame that the source
            // doesn't, itself, own, so the destination should just also refer
            // to the same (not-owned) frame
            dest.connectSocket_child_frame(srcChild);
        }
    }
}

bool osc::DeactivateAllWrapObjectsIn(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
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
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
            rv = rv || true;
        }
    }
    return rv;
}

void osc::InitalizeModel(OpenSim::Model& m)
{
    m.buildSystem();
    m.initializeState();
}

std::unique_ptr<OpenSim::Model> osc::CreateInitializedModelCopy(OpenSim::Model const& m)
{
    auto rv = std::make_unique<OpenSim::Model>(m);
    rv->buildSystem();
    rv->initializeState();
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

    AABB aabb = ses[0].worldspaceAABB;
    for (size_t i = 1; i < ses.size(); ++i)
    {
        aabb = Union(aabb, ses[i].worldspaceAABB);
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