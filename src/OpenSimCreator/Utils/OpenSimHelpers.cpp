#include "OpenSimHelpers.hpp"

#include <OpenSimCreator/Bindings/SimTKHelpers.hpp>
#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

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
#include <OpenSim/Simulation/Model/ModelComponent.h>
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
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Perf.hpp>
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
#include <span>
#include <utility>
#include <vector>

using osc::Vec3;

namespace
{
    constexpr Vec3 c_ContactHalfSpaceUpwardsNormal = {-1.0f, 0.0f, 0.0f};
}

// helpers
namespace
{
    // try to delete an item from an OpenSim::Set
    //
    // returns `true` if the item was found and deleted; otherwise, returns `false`
    template<typename T, typename TSetBase = OpenSim::Object>
    bool TryDeleteItemFromSet(OpenSim::Set<T, TSetBase>& set, T const* item)
    {
        for (size_t i = 0; i < osc::size(set); ++i)
        {
            if (&osc::At(set, i) == item)
            {
                return osc::EraseAt(set, i);
            }
        }
        return false;
    }

    bool IsConnectedViaSocketTo(OpenSim::Component const& c, OpenSim::Component const& other)
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

    std::vector<OpenSim::Component const*> GetAnyComponentsConnectedViaSocketTo(
        OpenSim::Component const& root,
        OpenSim::Component const& component)
    {
        std::vector<OpenSim::Component const*> rv;

        if (IsConnectedViaSocketTo(root, component))
        {
            rv.push_back(&root);
        }

        for (OpenSim::Component const& modelComponent : root.getComponentList())
        {
            if (IsConnectedViaSocketTo(modelComponent, component))
            {
                rv.push_back(&modelComponent);
            }
        }

        return rv;
    }

    std::vector<OpenSim::Component const*> GetAnyNonChildrenComponentsConnectedViaSocketTo(
        OpenSim::Component const& root,
        OpenSim::Component const& component)
    {
        std::vector<OpenSim::Component const*> allConnectees = GetAnyComponentsConnectedViaSocketTo(root, component);
        std::erase_if(allConnectees, [&root, &component](OpenSim::Component const* connectee)
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
        rv.reserve(osc::size(pfds));
        for (size_t i = 0; i < osc::size(pfds); ++i)
        {
            rv.emplace_back(osc::At(pfds, i));
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

    Vec3 GetLocationInGround(OpenSim::PointForceDirection& pf, SimTK::State const& st)
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

        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.first && attachmentIndexRange.first < std::ssize(pfds));
        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.second && attachmentIndexRange.second < std::ssize(pfds));

        if (attachmentIndexRange.first >= attachmentIndexRange.second)
        {
            return std::nullopt;  // not enough *unique* PFDs to compute a line of action
        }

        Vec3 const originPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first), st);
        Vec3 const pointAfterOriginPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first + 1), st);
        Vec3 const originDir = osc::Normalize(pointAfterOriginPos - originPos);

        Vec3 const insertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second), st);
        Vec3 const pointAfterInsertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second - 1), st);
        Vec3 const insertionDir = osc::Normalize(pointAfterInsertionPos - insertionPos);

        return osc::LinesOfAction
        {
            osc::PointDirection{originPos, originDir},
            osc::PointDirection{insertionPos, insertionDir},
        };
    }

    bool TryConnectTo(
        OpenSim::AbstractSocket& socket,
        OpenSim::Component const& c)
    {
        if (!socket.canConnectTo(c))
        {
            return false;
        }

        try
        {
            socket.connect(c);
            return true;
        }
        catch (OpenSim::Exception const&)
        {
            return false;
        }
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

OpenSim::Component* osc::UpdOwner(OpenSim::Component& root, OpenSim::Component const& c)
{
    if (auto const* constOwner = GetOwner(c))
    {
        return FindComponentMut(root, osc::GetAbsolutePath(*constOwner));
    }
    else
    {
        return nullptr;
    }
}

OpenSim::Component const& osc::GetOwnerOrThrow(OpenSim::AbstractOutput const& ao)
{
    return ao.getOwner();
}

OpenSim::Component const& osc::GetOwnerOrThrow(OpenSim::Component const& c)
{
    return c.getOwner();
}

OpenSim::Component const& osc::GetOwnerOr(OpenSim::Component const& c, OpenSim::Component const& fallback)
{
    return c.hasOwner() ? c.getOwner() : fallback;
}

OpenSim::Component const* osc::GetOwner(OpenSim::Component const& c)
{
    return c.hasOwner() ? &c.getOwner() : nullptr;
}

std::optional<std::string> osc::TryGetOwnerName(OpenSim::Component const& c)
{
    OpenSim::Component const* owner = GetOwner(c);
    return owner ? owner->getName() : std::optional<std::string>{};
}

size_t osc::DistanceFromRoot(OpenSim::Component const& c)
{
    size_t dist = 0;
    for (OpenSim::Component const* p = &c; p; p = GetOwner(*p))
    {
        ++dist;
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

    for (OpenSim::Component const* p = &c; p; p = GetOwner(*p))
    {
        rv.push_back(p);
    }

    std::reverse(rv.begin(), rv.end());

    return rv;
}

size_t osc::GetNumChildren(OpenSim::Component const& c)
{
    size_t rv = 0;
    for (OpenSim::Component const& descendant : c.getComponentList())
    {
        if (&descendant.getOwner() == &c)
        {
            ++rv;
        }
    }
    return rv;
}

bool osc::IsInclusiveChildOf(OpenSim::Component const* parent, OpenSim::Component const* c)
{
    if (parent == nullptr)
    {
        return false;
    }

    for (; c != nullptr; c = GetOwner(*c))
    {
        if (c == parent)
        {
            return true;
        }
    }

    return false;
}

OpenSim::Component const* osc::IsInclusiveChildOf(std::span<OpenSim::Component const*> parents, OpenSim::Component const* c)
{
    // TODO: this method signature makes no sense and should be refactored
    for (; c; c = GetOwner(*c))
    {
        if (auto it = std::find(parents.begin(), parents.end(), c); it != parents.end())
        {
            return *it;
        }
    }
    return nullptr;
}

OpenSim::Component const* osc::FindFirstAncestorInclusive(OpenSim::Component const* c, bool(*pred)(OpenSim::Component const*))
{
    for (; c; c = GetOwner(*c))
    {
        if (pred(c))
        {
            return c;
        }
    }
    return nullptr;
}

OpenSim::Component const* osc::FindFirstDescendentInclusive(
    OpenSim::Component const& component,
    bool(*predicate)(OpenSim::Component const&))
{
    if (predicate(component))
    {
        return &component;
    }
    else
    {
        return FindFirstDescendent(component, predicate);
    }
}

OpenSim::Component const* osc::FindFirstDescendent(
    OpenSim::Component const& component,
    bool(*predicate)(OpenSim::Component const&))
{
    for (OpenSim::Component const& descendent : component.getComponentList())
    {
        if (predicate(descendent))
        {
            return &descendent;
        }
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
    out.reserve(out.size() + size(s));

    for (size_t i = 0; i < size(s); ++i)
    {
        out.push_back(&At(s, i));
    }
}

float osc::ConvertCoordValueToDisplayValue(OpenSim::Coordinate const& c, double v)
{
    auto rv = static_cast<float>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = Rad2Deg(rv);
    }

    return rv;
}

double osc::ConvertCoordDisplayValueToStorageValue(OpenSim::Coordinate const& c, float v)
{
    auto rv = static_cast<double>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = Deg2Rad(rv);
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
        return {};
    }
}

std::vector<std::string> osc::GetSocketNames(OpenSim::Component const& c)
{
    return c.getSocketNames();
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

std::vector<OpenSim::AbstractSocket*> osc::UpdAllSockets(OpenSim::Component& c)
{
    std::vector<OpenSim::AbstractSocket*> rv;

    for (std::string const& name : GetSocketNames(c))
    {
        rv.push_back(&c.updSocket(name));
    }

    return rv;
}

OpenSim::Component const* osc::FindComponent(
    OpenSim::Component const& root,
    OpenSim::ComponentPath const& cp)
{
    if (IsEmpty(cp))
    {
        return nullptr;
    }

    try
    {
        return &root.getComponent(cp);
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
    return FindComponent(model, OpenSim::ComponentPath{absPath});
}

OpenSim::Component* osc::FindComponentMut(
    OpenSim::Component& root,
    OpenSim::ComponentPath const& cp)
{
    return const_cast<OpenSim::Component*>(FindComponent(root, cp));
}

bool osc::ContainsComponent(
    OpenSim::Component const& root,
    OpenSim::ComponentPath const& cp)
{
    return FindComponent(root, cp) != nullptr;
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

bool osc::IsConnectedTo(
    OpenSim::AbstractSocket const& s,
    OpenSim::Component const& c)
{
    return &s.getConnecteeAsObject() == &c;
}

bool osc::IsAbleToConnectTo(
    OpenSim::AbstractSocket const& s,
    OpenSim::Component const& c)
{
    return s.canConnectTo(c);
}

void osc::RecursivelyReassignAllSockets(
    OpenSim::Component& root,
    OpenSim::Component const& from,
    OpenSim::Component const& to)
{
    for (OpenSim::Component& c : root.updComponentList())
    {
        for (OpenSim::AbstractSocket* socket : osc::UpdAllSockets(c))
        {
            if (IsConnectedTo(*socket, from))
            {
                TryConnectTo(*socket, to);
            }
        }
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
    if (typeid(c) == typeid(OpenSim::PathWrapPoint))
    {
        return false;
    }
    else if (typeid(c) == typeid(OpenSim::Station) && OwnerIs<OpenSim::PathPoint>(c))
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
    OpenSim::Component* const owner = osc::UpdOwner(m, c);

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
        CStringView delim;
        for (OpenSim::Component const* connectee : connectees)
        {
            ss << delim << connectee->getName();
            delim = ", ";
        }
        log::error("cannot delete %s: the following components connect to it via sockets: %s", c.getName().c_str(), std::move(ss).str().c_str());
        return false;
    }

    // HACK: check if any path wraps connect to the component
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
    if (auto* componentSet = dynamic_cast<OpenSim::ComponentSet*>(owner))
    {
        rv = TryDeleteItemFromSet<OpenSim::ModelComponent, OpenSim::ModelComponent>(*componentSet, dynamic_cast<OpenSim::ModelComponent*>(&c));
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
    else if (auto* geom = dynamic_cast<OpenSim::Geometry*>(&c))
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

            auto& prop = dynamic_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(frame->updProperty_attached_geometry());
            auto copy = osc::Clone(prop);
            copy->clear();

            for (int i = 0; i < prop.size(); ++i)
            {
                if (OpenSim::Geometry& g = prop[i]; &g != geom)
                {
                    Append(*copy, g);
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
        for (size_t i = 0; i < size(wos); ++i)
        {
            OpenSim::WrapObject& wo = At(wos, i);
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
            rv = true;
        }
    }
    return rv;
}

bool osc::ActivateAllWrapObjectsIn(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>())
    {
        for (size_t i = 0; i < size(wos); ++i)
        {
            OpenSim::WrapObject& wo = At(wos, i);
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
            rv = true;
        }
    }
    return rv;
}

std::unique_ptr<osc::UndoableModelStatePair> osc::LoadOsimIntoUndoableModel(std::filesystem::path const& p)
{
    return std::make_unique<osc::UndoableModelStatePair>(p);
}

void osc::InitializeModel(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeModel");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)
    model.clearConnections();        // clears any potentially stale pointers that can be retained by OpenSim::Socket<T> (see #263)
    model.buildSystem();             // creates a new underlying physics system

    // HACK: ensure OpenSim::Coordinate::getMinRange()/getMaxRange() are callable without
    //       throwing exceptions (i.e. catch it here, rather than at some undetermined
    //       later time)
    //
    // see #654
    for (OpenSim::Coordinate const& c : model.getComponentList<OpenSim::Coordinate>())
    {
        c.getRangeMin();
        c.getRangeMax();
    }
}

void osc::FinalizeConnections(OpenSim::Model& model)
{
    OSC_PERF("osc::FinalizeConnections");
    model.finalizeConnections();
}

SimTK::State& osc::InitializeState(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeState");
    SimTK::State& state = model.initializeState();  // creates+returns a new working state
    model.equilibrateMuscles(state);
    model.realizeDynamics(state);
    return state;
}

void osc::FinalizeFromProperties(OpenSim::Model& model)
{
    OSC_PERF("osc::FinalizeFromProperties");
    model.finalizeFromProperties();
}

std::optional<size_t> osc::FindJointInParentJointSet(OpenSim::Joint const& joint)
{
    auto const* parentJointset = osc::GetOwner<OpenSim::JointSet>(joint);
    if (!parentJointset)
    {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return std::nullopt;
    }

    OpenSim::JointSet const& js = *parentJointset;
    for (size_t i = 0; i < size(js); ++i)
    {
        if (&At(js, i) == &joint)
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
    if (auto const* mesh = dynamic_cast<OpenSim::Mesh const*>(&g); mesh)
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

OpenSim::Appearance const* osc::TryGetAppearance(OpenSim::Component const& component)
{
    if (!component.hasProperty("Appearance"))
    {
        return nullptr;
    }

    OpenSim::AbstractProperty const& abstractProperty = component.getPropertyByName("Appearance");
    auto const* maybeAppearanceProperty = dynamic_cast<OpenSim::Property<OpenSim::Appearance> const*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->getValue() : nullptr;
}

OpenSim::Appearance* osc::TryUpdAppearance(OpenSim::Component& component)
{
    if (!component.hasProperty("Appearance"))
    {
        return nullptr;
    }

    OpenSim::AbstractProperty& abstractProperty = component.updPropertyByName("Appearance");
    auto* maybeAppearanceProperty = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->updValue() : nullptr;
}

bool osc::TrySetAppearancePropertyIsVisibleTo(OpenSim::Component& c, bool v)
{
    if (OpenSim::Appearance* appearance = TryUpdAppearance(c))
    {
        appearance->set_visible(v);
        return true;
    }
    else
    {
        return false;
    }
}

osc::Color osc::GetSuggestedBoneColor()
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
    std::array<OpenSim::Component const*, c_MaxEls> els{};
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
    rv.reserve(size(pps));  // best guess: but path wrapping might add more

    for (size_t i = 0; i < size(pps); ++i)
    {
        OpenSim::AbstractPathPoint const* const ap = At(pps, i);

        if (ap == nullptr)
        {
            continue;
        }
        else if (auto const* pwp = dynamic_cast<OpenSim::PathWrapPoint const*>(ap))
        {
            // special case: it's a wrapping point, so add each part of the wrap
            Transform const body2ground = ToTransform(pwp->getParentFrame().getTransformInGround(st));
            OpenSim::Array<SimTK::Vec3> const& wrapPath = pwp->getWrapPath(st);

            rv.reserve(rv.size() + size(wrapPath));
            for (size_t j = 0; j < size(wrapPath); ++j)
            {
                rv.emplace_back(body2ground * ToVec3(At(wrapPath, j)));
            }
        }
        else
        {
            rv.emplace_back(*ap, ToVec3(ap->getLocationInGround(st)));
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
        if (osc::empty(paramSet))
        {
            return nullptr;  // edge-case: the force has no parameters
        }

        // linearly search for a ContactHalfSpace
        for (size_t i = 0; i < osc::size(paramSet); ++i)
        {
            OpenSim::HuntCrossleyForce::ContactParameters const& param = osc::At(paramSet, i);
            OpenSim::Property<std::string> const& geomProperty = param.getProperty_geometry();

            for (size_t j = 0; osc::size(geomProperty); ++j)
            {
                std::string const& geomNameOrPath = osc::At(geomProperty, j);
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
        Vec3 force;
        Vec3 torque;
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

        Vec3 const force
        {
            static_cast<float>(-forces[0]),
            static_cast<float>(-forces[1]),
            static_cast<float>(-forces[2]),
        };

        if (osc::Length2(force) < std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;  // edge-case: no force is actually being exherted
        }

        Vec3 const torque
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
        // go through the contact geometries that are attached to the force
        //
        // - if there's a plane, then the plane's location+normal are needed in order
        //   to figure out where the force is exherted
        osc::Transform const body2ground = osc::ToTransform(halfSpace.getFrame().getTransformInGround(state));
        osc::Transform const geom2body = osc::ToTransform(halfSpace.getTransform());

        Vec3 const originInGround = body2ground * osc::ToVec3(halfSpace.get_location());
        Vec3 const normalInGround = osc::Normalize(body2ground.rotation * geom2body.rotation) * c_ContactHalfSpaceUpwardsNormal;

        return osc::Plane{originInGround, normalInGround};
    }

    // helper: returns the location of the center of pressure of a force+torque on a plane, or
    //         std::nullopt if the to-be-drawn force vector is too small
    std::optional<Vec3> ComputeCenterOfPressure(
        osc::Plane const& plane,
        ForceTorque const& forceTorque,
        float minimumForce = std::numeric_limits<float>::epsilon())
    {
        float const forceScaler = osc::Dot(plane.normal, forceTorque.force);

        if (std::abs(forceScaler) < minimumForce)
        {
            // edge-case: the resulting force vector is too small
            return std::nullopt;
        }

        if (std::abs(osc::Dot(plane.normal, osc::Normalize(forceTorque.torque))) >= 1.0f - std::numeric_limits<float>::epsilon())
        {
            // pedantic: the resulting torque is aligned with the plane normal, making
            // the cross product undefined later
            return std::nullopt;
        }

        // this maths seems sketchy, it's inspired by SCONE/model_tools.cpp:GetPlaneCop but
        // it feels a bit like `p1` is always going to be zero
        Vec3 const pos = osc::Cross(plane.normal, forceTorque.torque) / forceScaler;
        Vec3 const posRelativeToPlaneOrigin = pos - plane.origin;
        float const p1 = osc::Dot(posRelativeToPlaneOrigin, plane.normal);
        float const p2 = forceScaler;

        return pos - (p1/p2)*forceTorque.force;
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

    std::optional<Vec3> const maybePosition = ComputeCenterOfPressure(contactPlaneInGround, forceTorque);
    if (!maybePosition)
    {
        return std::nullopt;  // the resulting force is too small
    }

    return osc::ForcePoint{forceTorque.force, *maybePosition};
}

bool osc::CanExtractPointInfoFrom(OpenSim::Component const& c, SimTK::State const& st)
{
    return TryExtractPointInfo(c, st) != std::nullopt;
}

std::optional<osc::PointInfo> osc::TryExtractPointInfo(
    OpenSim::Component const& c,
    SimTK::State const& st)
{
    if (dynamic_cast<OpenSim::PathWrapPoint const*>(&c))
    {
        // HACK: path wrap points don't update the cache correctly?
        return std::nullopt;
    }
    else if (auto const* station = dynamic_cast<OpenSim::Station const*>(&c))
    {
        // HACK: OpenSim redundantly stores path point information in a child called 'station'.
        // These must be filtered because, otherwise, the user will just see a bunch of
        // 'station' entries below each path point
        if (station->getName() == "station" && osc::OwnerIs<OpenSim::PathPoint>(*station))
        {
            return std::nullopt;
        }

        return PointInfo
        {
            ToVec3(station->get_location()),
            osc::GetAbsolutePath(station->getParentFrame()),
        };
    }
    else if (auto const* pp = dynamic_cast<OpenSim::PathPoint const*>(&c))
    {
        return PointInfo
        {
            ToVec3(pp->getLocation(st)),
            osc::GetAbsolutePath(pp->getParentFrame()),
        };
    }
    else if (auto const* point = dynamic_cast<OpenSim::Point const*>(&c))
    {
        return PointInfo
        {
            ToVec3(point->getLocationInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }
    else if (auto const* frame = dynamic_cast<OpenSim::Frame const*>(&c))
    {
        return PointInfo
        {
            ToVec3(frame->getPositionInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }
    else
    {
        return std::nullopt;
    }
}

OpenSim::Component& osc::AddComponentToAppropriateSet(OpenSim::Model& m, std::unique_ptr<OpenSim::Component> c)
{
    if (c == nullptr)
    {
        throw std::runtime_error{"nullptr passed to AddComponentToAppropriateSet"};
    }

    OpenSim::Component& rv = *c;

    if (dynamic_cast<OpenSim::Body*>(c.get()))
    {
        m.addBody(dynamic_cast<OpenSim::Body*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Joint*>(c.get()))
    {
        m.addJoint(dynamic_cast<OpenSim::Joint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Constraint*>(c.get()))
    {
        m.addConstraint(dynamic_cast<OpenSim::Constraint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Force*>(c.get()))
    {
        m.addForce(dynamic_cast<OpenSim::Force*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Probe*>(c.get()))
    {
        m.addProbe(dynamic_cast<OpenSim::Probe*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::ContactGeometry*>(c.get()))
    {
        m.addContactGeometry(dynamic_cast<OpenSim::ContactGeometry*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Marker*>(c.get()))
    {
        m.addMarker(dynamic_cast<OpenSim::Marker*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Controller*>(c.get()))
    {
        m.addController(dynamic_cast<OpenSim::Controller*>(c.release()));
    }
    else
    {
        m.addComponent(c.release());
    }

    return rv;
}

OpenSim::ModelComponent& osc::AddModelComponent(OpenSim::Model& model, std::unique_ptr<OpenSim::ModelComponent> p)
{
    OpenSim::ModelComponent& rv = *p;
    model.addModelComponent(p.release());
    return rv;
}

OpenSim::Component& osc::AddComponent(OpenSim::Component& c, std::unique_ptr<OpenSim::Component> p)
{
    OpenSim::Component& rv = *p;
    c.addComponent(p.release());
    return rv;
}

OpenSim::Body& osc::AddBody(OpenSim::Model& model, std::unique_ptr<OpenSim::Body> p)
{
    OpenSim::Body& rv = *p;
    model.addBody(p.release());
    return rv;
}

OpenSim::Joint& osc::AddJoint(OpenSim::Model& model, std::unique_ptr<OpenSim::Joint> j)
{
    OpenSim::Joint& rv = *j;
    model.addJoint(j.release());
    return rv;
}

OpenSim::Marker& osc::AddMarker(OpenSim::Model& model, std::unique_ptr<OpenSim::Marker> marker)
{
    OpenSim::Marker& rv = *marker;
    model.addMarker(marker.release());
    return rv;
}

OpenSim::PhysicalOffsetFrame& osc::AddFrame(OpenSim::Joint& joint, std::unique_ptr<OpenSim::PhysicalOffsetFrame> frame)
{
    OpenSim::PhysicalOffsetFrame& rv = *frame;
    joint.addFrame(frame.release());
    return rv;
}

OpenSim::Geometry& osc::AttachGeometry(OpenSim::Frame& frame, std::unique_ptr<OpenSim::Geometry> p)
{
    OpenSim::Geometry& rv = *p;
    frame.attachGeometry(p.release());
    return rv;
}

std::optional<SimTK::Transform> osc::TryGetParentToGroundTransform(
    OpenSim::Component const& component,
    SimTK::State const& state)
{
    if (auto const* station = dynamic_cast<OpenSim::Station const*>(&component))
    {
        return station->getParentFrame().getTransformInGround(state);
    }
    else if (auto const* pp = dynamic_cast<OpenSim::PathPoint const*>(&component))
    {
        return pp->getParentFrame().getTransformInGround(state);
    }
    else if (auto const* pof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&component))
    {
        return pof->getParentFrame().getTransformInGround(state);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::string> osc::TryGetPositionalPropertyName(
    OpenSim::Component const& component)
{
    if (auto const* station = dynamic_cast<OpenSim::Station const*>(&component))
    {
        return station->getProperty_location().getName();
    }
    else if (auto const* pp = dynamic_cast<OpenSim::PathPoint const*>(&component))
    {
        return pp->getProperty_location().getName();
    }
    else if (auto const* pof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&component))
    {
        return pof->getProperty_translation().getName();
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::string> osc::TryGetOrientationalPropertyName(
    OpenSim::Component const& component)
{
    if (auto const* pof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&component))
    {
        return pof->getProperty_orientation().getName();
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<osc::ComponentSpatialRepresentation> osc::TryGetSpatialRepresentation(
    OpenSim::Component const& component,
    SimTK::State const& state)
{
    if (auto xform = TryGetParentToGroundTransform(component, state))
    {
        if (auto posProp = TryGetPositionalPropertyName(component))
        {
            return ComponentSpatialRepresentation
            {
                *xform,
                std::move(posProp).value(),
                TryGetOrientationalPropertyName(component)
            };
        }
    }
    return std::nullopt;
}
