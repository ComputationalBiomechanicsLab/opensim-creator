#include "OpenSimHelpers.hpp"

#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"

#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Algorithms.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Perf.hpp>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Exception.h>
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
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ControllerSet.h>
#include <OpenSim/Simulation/Model/CoordinateSet.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/ForceSet.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/MarkerSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PathPointSet.h>
#include <OpenSim/Simulation/Model/PointForceDirection.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/Model/ProbeSet.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/Wrap/PathWrap.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <SimTKcommon.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

// helpers
namespace
{
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

    bool IsConnectedViaSocketTo(OpenSim::Component& c, OpenSim::Component const& other)
    {
        // TODO: .getSocketNames() should be `const` in OpenSim >4.4
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

    std::vector<OpenSim::Component*> GetAnyComponentsConnectedViaSocketTo(
        OpenSim::Component& root,
        OpenSim::Component const& component)
    {
        std::vector<OpenSim::Component*> rv;

        if (IsConnectedViaSocketTo(root, component))
        {
            rv.push_back(&root);
        }

        for (OpenSim::Component& modelComponent : root.updComponentList())
        {
            if (IsConnectedViaSocketTo(modelComponent, component))
            {
                rv.push_back(&modelComponent);
            }
        }

        return rv;
    }

    std::vector<OpenSim::Component*> GetAnyNonChildrenComponentsConnectedViaSocketTo(
        OpenSim::Component& root,
        OpenSim::Component const& component)
    {
        std::vector<OpenSim::Component*> allConnectees = GetAnyComponentsConnectedViaSocketTo(root, component);
        osc::RemoveErase(allConnectees, [&root, &component](OpenSim::Component* connectee)
        {
            return
                osc::IsInclusiveChildOf(&component, connectee) &&
                GetAnyComponentsConnectedViaSocketTo(root, *connectee).empty();  // care: the child may, itself, have things connected to it
        });
        return allConnectees;
    }

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

    // returns the index of the "effective" origin point of a muscle PFD sequence
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

    // returns the index of the "effective" insertion point of a muscle PFD sequence
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

    std::optional<osc::LinesOfAction> TryGetLinesOfAction(
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

        return osc::LinesOfAction
        {
            osc::PointDirection{originPos, originDir},
            osc::PointDirection{insertionPos, insertionDir},
        };
    }
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
    static OpenSim::ComponentPath const s_EmptyComponentPath;
    return s_EmptyComponentPath;
}

OpenSim::ComponentPath const& osc::GetRootComponentPath()
{
    static OpenSim::ComponentPath const s_RootComponentPath{"/"};
    return s_RootComponentPath;
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

void osc::GetCoordinatesInModel(
    OpenSim::Model const& m,
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

OpenSim::Component const* osc::FindComponent(
    OpenSim::Component const& c,
    OpenSim::ComponentPath const& cp)
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

OpenSim::Component const* osc::FindComponent(
    OpenSim::Model const& model,
    std::string const& absPath)
{
    return osc::FindComponent(model, OpenSim::ComponentPath{absPath});
}

OpenSim::Component* osc::FindComponentMut(
    OpenSim::Component& c,
    OpenSim::ComponentPath const& cp)
{
    return const_cast<OpenSim::Component*>(FindComponent(c, cp));
}

bool osc::ContainsComponent(
    OpenSim::Component const& root,
    OpenSim::ComponentPath const& cp)
{
    return FindComponent(root, cp);
}

OpenSim::AbstractSocket const* osc::FindSocket(
    OpenSim::Component const& c,
    std::string const& name)
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

OpenSim::AbstractSocket* osc::FindSocketMut(
    OpenSim::Component& c,
    std::string const& name)
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

bool osc::IsAbleToConnectTo(
    OpenSim::AbstractSocket const& s,
    OpenSim::Component const& c)
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

OpenSim::AbstractProperty* osc::FindPropertyMut(
    OpenSim::Component& c,
    std::string const& name)
{
    return c.hasProperty(name) ? &c.updPropertyByName(name) : nullptr;
}

OpenSim::AbstractOutput const* osc::FindOutput(
    OpenSim::Component const& c,
    std::string const& outputName)
{
    OpenSim::AbstractOutput const* rv = nullptr;
    try
    {
        rv = &c.getOutput(outputName);
    }
    catch (OpenSim::Exception const&)
    {
        // OpenSim, innit :(
    }
    return rv;
}

OpenSim::AbstractOutput const* osc::FindOutput(
    OpenSim::Component const& root,
    OpenSim::ComponentPath const& path,
    std::string const& outputName)
{
    OpenSim::Component const* const c = FindComponent(root, path);
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
    OpenSim::Component* const owner = osc::UpdOwner(c);

    if (!owner)
    {
        log::error("cannot delete %s: it has no owner", c.getName().c_str());
        return false;
    }

    if (&c.getRoot() != &m)
    {
        log::error("cannot delete %s: it is not owned by the provided model", c.getName().c_str());
        return false;
    }

    // check if anything connects to the component as a non-child (i.e. non-hierarchically)
    // via a socket to the component, which may break the other component (so halt deletion)
    if (auto connectees = GetAnyNonChildrenComponentsConnectedViaSocketTo(m, c); !connectees.empty())
    {
        std::stringstream ss;
        CStringView delim = "";
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
            log::error("cannot delete %s: it is used in a path wrap (%s)", c.getName().c_str(), osc::GetAbsolutePathString(pw).c_str());
            return false;
        }
    }

    // at this point we know that it's *technically* feasible to delete the component
    // from the model without breaking sockets etc., so now we use heuristics to figure
    // out how to do that

    bool rv = false;

    // disable deleting joints: it's super-easy to segfault because of some unknown
    // fuckery happening in OpenSim::Model::createMultibodySystem
    // if (auto* js = dynamic_cast<OpenSim::JointSet*>(owner))
    // {
    //    rv = TryDeleteItemFromSet(*js, dynamic_cast<OpenSim::Joint*>(&c));
    // }
    if (auto* cs = dynamic_cast<OpenSim::ComponentSet*>(owner))
    {
        rv = TryDeleteItemFromSet<OpenSim::ModelComponent, OpenSim::ModelComponent>(*cs, dynamic_cast<OpenSim::ModelComponent*>(&c));
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
                dynamic_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(frame->updProperty_attached_geometry());

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
    else if (OpenSim::Joint* j = dynamic_cast<OpenSim::Joint*>(c.get()))
    {
        // HOTFIX: `OpenSim::Ground` should never be listed as a joint's parent, because it
        //         causes a segfault in OpenSim 4.4 (#543)
        if (dynamic_cast<OpenSim::Ground const*>(&j->getChildFrame()))
        {
            throw std::runtime_error{"cannot create a new joint with 'ground' as the child: did you mix up parent/child?"};
        }

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

std::unique_ptr<osc::UndoableModelStatePair> osc::LoadOsimIntoUndoableModel(std::filesystem::path const& p)
{
    return std::make_unique<osc::UndoableModelStatePair>(p);
}

void osc::InitializeModel(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeModel");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)

    // HACK: reset body inertias to NaN to force OpenSim (at least <= 4.4) to recompute
    //       inertia correctly (#597)
    //
    // this is necessary, because OpenSim contains a bug where body inertias aren't updated
    // by property changes (see #597 for a better explanation)
    for (OpenSim::Body& body : model.updComponentList<OpenSim::Body>())
    {
        // maintain copy of user-enacted/osim inertia value
        SimTK::Vec6 const userInertia = body.get_inertia();

        // NaN both the property and the internal `_inertia` member
        body.setInertia(SimTK::Inertia{});

        // but then reset the property to the user-enacted value
        body.set_inertia(userInertia);

        // (which should cause OpenSim to behave as-if finalized properly)
    }

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

osc::CStringView osc::GetMotionTypeDisplayName(OpenSim::Coordinate const& c)
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

osc::Color osc::GetSuggestedBoneColor() noexcept
{
    Color usualDefault = {232.0f / 255.0f, 216.0f / 255.0f, 200.0f/255.0f, 1.0f};
    float brightenAmount = 0.1f;
    return Lerp(usualDefault, Color::white(), brightenAmount);
}

bool osc::IsShowingFrames(OpenSim::Model const& model)
{
    return model.getDisplayHints().get_show_frames();
}

bool osc::ToggleShowingFrames(OpenSim::Model& model)
{
    bool const newValue = !IsShowingFrames(model);
    model.updDisplayHints().set_show_frames(newValue);
    return newValue;
}

bool osc::IsShowingMarkers(OpenSim::Model const& model)
{
    return model.getDisplayHints().get_show_markers();
}

bool osc::ToggleShowingMarkers(OpenSim::Model& model)
{
    bool const newValue = !IsShowingMarkers(model);
    model.updDisplayHints().set_show_markers(newValue);
    return newValue;
}

bool osc::IsShowingWrapGeometry(OpenSim::Model const& model)
{
    return model.getDisplayHints().get_show_wrap_geometry();
}

bool osc::ToggleShowingWrapGeometry(OpenSim::Model& model)
{
    bool const newValue = !IsShowingWrapGeometry(model);
    model.updDisplayHints().set_show_wrap_geometry(newValue);
    return newValue;
}

bool osc::IsShowingContactGeometry(OpenSim::Model const& model)
{
    return model.getDisplayHints().get_show_contact_geometry();
}

bool osc::ToggleShowingContactGeometry(OpenSim::Model& model)
{
    bool const newValue = !IsShowingContactGeometry(model);
    model.updDisplayHints().set_show_contact_geometry(newValue);
    return newValue;
}

void osc::GetAbsolutePathString(OpenSim::Component const& c, std::string& out)
{
    constexpr ptrdiff_t c_MaxEls = 16;

    ptrdiff_t nEls = 0;
    std::array<OpenSim::Component const*, c_MaxEls> els;
    OpenSim::Component const* cur = &c;
    OpenSim::Component const* next = osc::GetOwner(*cur);

    if (!next)
    {
        // edge-case: caller provided a root
        out = '/';
        return;
    }

    while (cur && next && nEls < c_MaxEls)
    {
        els[nEls++] = cur;
        cur = next;
        next = osc::GetOwner(*cur);
    }

    if (nEls >= c_MaxEls)
    {
        // edge-case: component is too deep: fallback to OpenSim impl.
        out = c.getAbsolutePathString();
        return;
    }

    // else: construct the path piece-by-piece

    // precompute path length (memory allocation)
    size_t pathlen = nEls;
    for (ptrdiff_t i = 0; i < nEls; ++i)
    {
        pathlen += els[i]->getName().size();
    }

    // then preallocate the string
    out.resize(pathlen);

    // and assign it
    size_t loc = 0;
    for (ptrdiff_t i = nEls-1; i >= 0; --i)
    {
        out[loc++] = '/';
        std::string const& name = els[i]->getName();
        std::copy(name.begin(), name.end(), out.begin() + loc);
        loc += name.size();
    }
}

std::string osc::GetAbsolutePathString(OpenSim::Component const& c)
{
    std::string rv;
    GetAbsolutePathString(c, rv);
    return rv;
}

OpenSim::ComponentPath osc::GetAbsolutePath(OpenSim::Component const& c)
{
    return OpenSim::ComponentPath{GetAbsolutePathString(c)};
}

OpenSim::ComponentPath osc::GetAbsolutePathOrEmpty(OpenSim::Component const* c)
{
    if (c)
    {
        return osc::GetAbsolutePath(*c);
    }
    else
    {
        return OpenSim::ComponentPath{};
    }
}

std::optional<osc::LinesOfAction> osc::GetEffectiveLinesOfActionInGround(
    OpenSim::Muscle const& muscle,
    SimTK::State const& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = true;
    return TryGetLinesOfAction(muscle, state, config);
}

std::optional<osc::LinesOfAction> osc::GetAnatomicalLinesOfActionInGround(
    OpenSim::Muscle const& muscle,
    SimTK::State const& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = false;
    return TryGetLinesOfAction(muscle, state, config);
}

std::vector<osc::GeometryPathPoint> osc::GetAllPathPoints(OpenSim::GeometryPath const& gp, SimTK::State const& st)
{
    OpenSim::Array<OpenSim::AbstractPathPoint*> const& pps = gp.getCurrentPath(st);

    std::vector<GeometryPathPoint> rv;
    rv.reserve(pps.getSize());  // best guess: but path wrapping might add more

    for (int i = 0; i < pps.getSize(); ++i)
    {

        OpenSim::AbstractPathPoint const& ap = *pps[i];

        if (typeid(ap) == typeid(OpenSim::PathWrapPoint))
        {
            // special case: it's a wrapping point, so add each part of the wrap
            OpenSim::PathWrapPoint const* pwp = static_cast<OpenSim::PathWrapPoint const*>(&ap);

            Transform const body2ground = ToTransform(pwp->getParentFrame().getTransformInGround(st));
            OpenSim::Array<SimTK::Vec3> const& wrapPath = pwp->getWrapPath(st);

            rv.reserve(rv.size() + wrapPath.getSize());
            for (int j = 0; j < wrapPath.getSize(); ++j)
            {
                rv.emplace_back(body2ground * ToVec3(wrapPath[j]));
            }
        }
        else
        {
            rv.emplace_back(ap, ToVec3(ap.getLocationInGround(st)));
        }
    }

    return rv;
}

namespace
{
    // returns the first ContactHalfSpace found within the given HuntCrossleyForce's parameters, or
    // nullptr, if no ContactHalfSpace could be found
    OpenSim::ContactHalfSpace const* TryFindFirstContactHalfSpace(
        OpenSim::Model const& model,
        OpenSim::HuntCrossleyForce const& hcf)
    {
        // get contact parameters (i.e. where the contact geometry is stored)
        OpenSim::HuntCrossleyForce::ContactParametersSet const& paramSet = hcf.get_contact_parameters();
        if (paramSet.getSize() <= 0)
        {
            return nullptr;  // edge-case: the force has no parameters
        }

        // linearly search for a ContactHalfSpace
        for (int i = 0; i < paramSet.getSize(); ++i)
        {
            OpenSim::HuntCrossleyForce::ContactParameters const& param = paramSet[i];
            OpenSim::Property<std::string> const& geomProperty = param.getProperty_geometry();
            for (int j = 0; j < geomProperty.size(); ++j)
            {
                std::string const& geomNameOrPath = geomProperty[j];
                if (auto const* foundViaAbsPath = osc::FindComponent<OpenSim::ContactHalfSpace>(model, geomNameOrPath))
                {
                    // found it as an abspath within the model
                    return foundViaAbsPath;
                }
                else if (auto const* foundViaRelativePath = osc::FindComponent<OpenSim::ContactHalfSpace>(model.getContactGeometrySet(), geomNameOrPath))
                {
                    // found it as a relative path/name within the contactgeometryset
                    return foundViaRelativePath;
                }
            }
        }
        return nullptr;
    }

    // helper: try to extract the current (state-dependent) force+torque from a HuntCrossleyForce
    struct ForceTorque final {
        glm::vec3 force;
        glm::vec3 torque;
    };
    std::optional<ForceTorque> TryComputeCurrentForceTorque(
        OpenSim::HuntCrossleyForce const& hcf,
        SimTK::State const& state)
    {
        OpenSim::Array<double> const forces = hcf.getRecordValues(state);
        if (forces.size() < 6)
        {
            return std::nullopt;  // edge-case: OpenSim didn't report the expected forces
        }

        glm::vec3 const force
        {
            static_cast<float>(-forces[0]),
            static_cast<float>(-forces[1]),
            static_cast<float>(-forces[2]),
        };

        if (glm::length2(force) < std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;  // edge-case: no force is actually being exherted
        }

        glm::vec3 const torque
        {
            static_cast<float>(-forces[3]),
            static_cast<float>(-forces[4]),
            static_cast<float>(-forces[5]),
        };

        return ForceTorque{force, torque};
    }

    // helper: convert an OpenSim::ContactHalfSpace, which is defined in a frame with an offset,
    //         etc. into a simpler "plane in groundspace" representation that's more useful
    //         for rendering
    osc::Plane ToAnalyticPlaneInGround(
        OpenSim::ContactHalfSpace const& halfSpace,
        SimTK::State const& state)
    {
        glm::vec3 constexpr c_ContactHalfSpaceUpwardsNormal = {-1.0f, 0.0f, 0.0f};

        // go through the contact geometries that are attached to the force
        //
        // - if there's a plane, then the plane's location+normal are needed in order
        //   to figure out where the force is exherted
        osc::Transform const body2ground = osc::ToTransform(halfSpace.getFrame().getTransformInGround(state));
        osc::Transform const geom2body = osc::ToTransform(halfSpace.getTransform());

        glm::vec3 const originInGround = body2ground * osc::ToVec3(halfSpace.get_location());
        glm::vec3 const normalInGround = glm::normalize(body2ground.rotation * geom2body.rotation) * c_ContactHalfSpaceUpwardsNormal;

        return osc::Plane{originInGround, normalInGround};
    }

    // helper: returns the location of the center of pressure of a force+torque on a plane, or
    //         std::nullopt if the to-be-drawn force vector is too small
    std::optional<glm::vec3> ComputeCenterOfPressure(
        osc::Plane const& plane,
        ForceTorque const& forceTorque,
        float minimumForce = std::numeric_limits<float>::epsilon())
    {
        float const forceScaler = glm::dot(plane.normal, forceTorque.force);

        if (std::abs(forceScaler) < minimumForce)
        {
            // edge-case: the resulting force vector is too small
            return std::nullopt;
        }

        if (std::abs(glm::dot(plane.normal, glm::normalize(forceTorque.torque))) >= 1.0f - std::numeric_limits<float>::epsilon())
        {
            // pedantic: the resulting torque is aligned with the plane normal, making
            // the cross product undefined later
            return std::nullopt;
        }

        // this maths seems sketchy, it's inspired by SCONE/model_tools.cpp:GetPlaneCop but
        // it feels a bit like `p1` is always going to be zero
        glm::vec3 const pos = glm::cross(plane.normal, forceTorque.torque) / forceScaler;
        glm::vec3 const posRelativeToPlaneOrigin = pos - plane.origin;
        float const p1 = glm::dot(posRelativeToPlaneOrigin, plane.normal);
        float const p2 = forceScaler;

        return pos - (p1/p2)*forceTorque.force;
    }

    std::optional<osc::ForcePoint> GetForceValueInGround(
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::HuntCrossleyForce const& hcf)
    {
        // try and find a contact half space to attach the force vectors to
        OpenSim::ContactHalfSpace const* const maybeContactHalfSpace = TryFindFirstContactHalfSpace(model, hcf);
        if (!maybeContactHalfSpace)
        {
            return std::nullopt;  // couldn't find a ContactHalfSpace
        }
        osc::Plane const contactPlaneInGround = ToAnalyticPlaneInGround(*maybeContactHalfSpace, state);

        // try and compute the force vectors
        std::optional<ForceTorque> const maybeForceTorque = TryComputeCurrentForceTorque(hcf, state);
        if (!maybeForceTorque)
        {
            return std::nullopt;  // couldn't extract force+torque from the HCF
        }
        ForceTorque const& forceTorque = *maybeForceTorque;

        std::optional<glm::vec3> maybePosition = ComputeCenterOfPressure(contactPlaneInGround, forceTorque);
        if (!maybePosition)
        {
            return std::nullopt;  // the resulting force is too small
        }

        return osc::ForcePoint{forceTorque.force, *maybePosition};
    }

    std::optional<osc::ForcePoint> TryGetForceValueInGround(
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::Force const& force)
    {
        if (auto const* hcf = dynamic_cast<OpenSim::HuntCrossleyForce const*>(&force))
        {
            return GetForceValueInGround(model, state, *hcf);
        }
        else
        {
            return std::nullopt;
        }
    }
}

std::optional<osc::ForcePoint> osc::TryGetContactForceInGround(
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::HuntCrossleyForce const& hcf)
{
    // try and find a contact half space to attach the force vectors to
    OpenSim::ContactHalfSpace const* const maybeContactHalfSpace = TryFindFirstContactHalfSpace(model, hcf);
    if (!maybeContactHalfSpace)
    {
        return std::nullopt;  // couldn't find a ContactHalfSpace
    }
    osc::Plane const contactPlaneInGround = ToAnalyticPlaneInGround(*maybeContactHalfSpace, state);

    // try and compute the force vectors
    std::optional<ForceTorque> const maybeForceTorque = TryComputeCurrentForceTorque(hcf, state);
    if (!maybeForceTorque)
    {
        return std::nullopt;  // couldn't extract force+torque from the HCF
    }
    ForceTorque const& forceTorque = *maybeForceTorque;

    std::optional<glm::vec3> const maybePosition = ComputeCenterOfPressure(contactPlaneInGround, forceTorque);
    if (!maybePosition)
    {
        return std::nullopt;  // the resulting force is too small
    }

    return osc::ForcePoint{forceTorque.force, *maybePosition};
}
