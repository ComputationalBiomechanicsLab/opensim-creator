#include "OpenSimHelpers.h"

#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Common/TableUtilities.h>
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
#include <OpenSim/Simulation/Model/ExternalForce.h>
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
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar_simbody/SimTKHelpers.h>
#include <SimTKcommon.h>
#include <SimTKcommon/SmallMatrix.h>

#include <cmath>
#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ostream>
#include <set>
#include <span>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

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
    bool TryDeleteItemFromSet(OpenSim::Set<T, TSetBase>& set, const T* item)
    {
        for (size_t i = 0; i < size(set); ++i) {
            if (&At(set, i) == item) {
                return EraseAt(set, i);
            }
        }
        return false;
    }

    bool IsConnectedViaSocketTo(const OpenSim::Component& c, const OpenSim::Component& other)
    {
        for (const std::string& socketName : c.getSocketNames()) {
            const OpenSim::AbstractSocket& sock = c.getSocket(socketName);
            if (sock.isConnected() && &sock.getConnecteeAsObject() == &other) {
                return true;
            }
        }
        return false;
    }

    std::vector<const OpenSim::Component*> GetAnyComponentsConnectedViaSocketTo(
        const OpenSim::Component& root,
        const OpenSim::Component& component)
    {
        std::vector<const OpenSim::Component*> rv;

        if (IsConnectedViaSocketTo(root, component)) {
            rv.push_back(&root);
        }

        for (const OpenSim::Component& modelComponent : root.getComponentList()) {
            if (IsConnectedViaSocketTo(modelComponent, component)) {
                rv.push_back(&modelComponent);
            }
        }

        return rv;
    }

    std::vector<const OpenSim::Component*> GetAnyNonChildrenComponentsConnectedViaSocketTo(
        const OpenSim::Component& root,
        const OpenSim::Component& component)
    {
        std::vector<const OpenSim::Component*> allConnectees = GetAnyComponentsConnectedViaSocketTo(root, component);
        std::erase_if(allConnectees, [&root, &component](const OpenSim::Component* connectee)
        {
            return
                IsInclusiveChildOf(&component, connectee) &&
                GetAnyComponentsConnectedViaSocketTo(root, *connectee).empty();  // care: the child may, itself, have things connected to it
        });
        return allConnectees;
    }

    // returns the index of the "effective" origin point of a muscle PFD sequence
    ptrdiff_t GetEffectiveOrigin(const std::vector<std::unique_ptr<OpenSim::PointForceDirection>>& pfds)
    {
        OSC_ASSERT_ALWAYS(!pfds.empty());

        // move forward through the PFD sequence until a different frame is found
        //
        // the PFD before that one is the effective origin
        const auto it = find_if(
            pfds.begin() + 1,
            pfds.end(),
            [&first = pfds.front()](const auto& pfd) { return &pfd->frame() != &first->frame(); }
        );
        return std::distance(pfds.begin(), it) - 1;
    }

    // returns the index of the "effective" insertion point of a muscle PFD sequence
    ptrdiff_t GetEffectiveInsertion(const std::vector<std::unique_ptr<OpenSim::PointForceDirection>>& pfds)
    {
        OSC_ASSERT_ALWAYS(!pfds.empty());

        // move backward through the PFD sequence until a different frame is found
        //
        // the PFD after that one is the effective insertion
        const auto rit = find_if(
            pfds.rbegin() + 1,
            pfds.rend(),
            [&last = pfds.back()](const auto& pfd) { return &pfd->frame() != &last->frame(); }
        );
        return std::distance(pfds.begin(), rit.base());
    }

    // returns an index range into the provided array that contains only
    // effective attachment points? (see: https://github.com/modenaxe/MuscleForceDirection/blob/master/CPP/MuscleForceDirection/MuscleForceDirection.cpp)
    std::pair<ptrdiff_t, ptrdiff_t> GetEffectiveAttachmentIndices(const std::vector<std::unique_ptr<OpenSim::PointForceDirection>>& pfds)
    {
        return {GetEffectiveOrigin(pfds), GetEffectiveInsertion(pfds)};
    }

    std::pair<ptrdiff_t, ptrdiff_t> GetAnatomicalAttachmentIndices(const std::vector<std::unique_ptr<OpenSim::PointForceDirection>>& pfds)
    {
        OSC_ASSERT(!pfds.empty());

        return {0, pfds.size() - 1};
    }

    Vec3 GetLocationInGround(OpenSim::PointForceDirection& pf, const SimTK::State& st)
    {
        const SimTK::Vec3 location = pf.frame().findStationLocationInGround(st, pf.point());
        return to<Vec3>(location);
    }

    struct LinesOfActionConfig final {

        // as opposed to using "anatomical"
        bool useEffectiveInsertion = true;
    };

    std::optional<LinesOfAction> TryGetLinesOfAction(
        const OpenSim::Muscle& muscle,
        const SimTK::State& st,
        const LinesOfActionConfig& config)
    {
        const std::vector<std::unique_ptr<OpenSim::PointForceDirection>> pfds = GetPointForceDirections(muscle.getGeometryPath(), st);
        if (pfds.size() < 2) {
            return std::nullopt;  // not enough PFDs to compute a line of action
        }

        const std::pair<ptrdiff_t, ptrdiff_t> attachmentIndexRange = config.useEffectiveInsertion ?
            GetEffectiveAttachmentIndices(pfds) :
            GetAnatomicalAttachmentIndices(pfds);

        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.first && attachmentIndexRange.first < std::ssize(pfds));
        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.second && attachmentIndexRange.second < std::ssize(pfds));

        if (attachmentIndexRange.first >= attachmentIndexRange.second) {
            return std::nullopt;  // not enough *unique* PFDs to compute a line of action
        }

        const Vec3 originPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first), st);
        const Vec3 pointAfterOriginPos = GetLocationInGround(*pfds.at(attachmentIndexRange.first + 1), st);
        const Vec3 originDir = normalize(pointAfterOriginPos - originPos);

        const Vec3 insertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second), st);
        const Vec3 pointAfterInsertionPos = GetLocationInGround(*pfds.at(attachmentIndexRange.second - 1), st);
        const Vec3 insertionDir = normalize(pointAfterInsertionPos - insertionPos);

        return LinesOfAction{
            PointDirection{originPos, originDir},
            PointDirection{insertionPos, insertionDir},
        };
    }

    bool TryConnectTo(
        OpenSim::AbstractSocket& socket,
        const OpenSim::Component& c)
    {
        if (socket.canConnectTo(c)) {
            socket.connect(c);
            return true;
        }
        else {
            return false;
        }
    }

    OpenSim::Component& GetOrUpdComponent(OpenSim::Component& c, const OpenSim::ComponentPath& cp)
    {
        return c.updComponent(cp);
    }

    const OpenSim::Component& GetOrUpdComponent(const OpenSim::Component& c, const OpenSim::ComponentPath& cp)
    {
        return c.getComponent(cp);
    }

    template<std::derived_from<OpenSim::Component> Component>
    Component* FindComponentGeneric(Component& c, const OpenSim::ComponentPath& cp)
    {
        if (cp == OpenSim::ComponentPath{}) {
            return nullptr;
        }

        try {
            return &GetOrUpdComponent(c, cp);
        }
        catch (const OpenSim::Exception&) {
            return nullptr;
        }
    }
}


// public API

bool osc::IsConcreteClassNameLexographicallyLowerThan(const OpenSim::Component& a, const OpenSim::Component& b)
{
    return a.getConcreteClassName() < b.getConcreteClassName();
}

bool osc::IsNameLexographicallyLowerThan(const OpenSim::Component& a, const OpenSim::Component& b)
{
    return a.getName() < b.getName();
}

OpenSim::Component* osc::UpdOwner(OpenSim::Component& root, const OpenSim::Component& c)
{
    if (const auto* constOwner = GetOwner(c)) {
        return FindComponentMut(root, GetAbsolutePath(*constOwner));
    }
    else {
        return nullptr;
    }
}

OpenSim::Component& osc::UpdOwnerOrThrow(OpenSim::Component& root, const OpenSim::Component& c)
{
    auto* p = UpdOwner(root, c);
    if (not p) {
        throw std::runtime_error{"could not update a component's owner"};
    }
    return *p;
}

const OpenSim::Component& osc::GetOwnerOrThrow(const OpenSim::AbstractOutput& ao)
{
    return ao.getOwner();
}

const OpenSim::Component& osc::GetOwnerOrThrow(const OpenSim::Component& c)
{
    return c.getOwner();
}

const OpenSim::Component& osc::GetOwnerOr(const OpenSim::Component& c, const OpenSim::Component& fallback)
{
    return c.hasOwner() ? c.getOwner() : fallback;
}

const OpenSim::Component* osc::GetOwner(const OpenSim::Component& c)
{
    return c.hasOwner() ? &c.getOwner() : nullptr;
}

std::optional<std::string> osc::TryGetOwnerName(const OpenSim::Component& c)
{
    const OpenSim::Component* owner = GetOwner(c);
    return owner ? owner->getName() : std::optional<std::string>{};
}

size_t osc::DistanceFromRoot(const OpenSim::Component& c)
{
    size_t dist = 0;
    for (const OpenSim::Component* p = &c; p; p = GetOwner(*p)) {
        ++dist;
    }
    return dist;
}

OpenSim::ComponentPath osc::GetRootComponentPath()
{
    return OpenSim::ComponentPath{"/"};
}

bool osc::IsEmpty(const OpenSim::ComponentPath& cp)
{
    return cp == OpenSim::ComponentPath{};
}

void osc::Clear(OpenSim::ComponentPath& cp)
{
    cp = OpenSim::ComponentPath{};
}

std::vector<const OpenSim::Component*> osc::GetPathElements(const OpenSim::Component& c)
{
    std::vector<const OpenSim::Component*> rv;
    rv.reserve(DistanceFromRoot(c));

    for (const OpenSim::Component* p = &c; p; p = GetOwner(*p)) {
        rv.push_back(p);
    }

    rgs::reverse(rv);

    return rv;
}

void osc::ForEachComponent(
    const OpenSim::Component& component,
    const std::function<void(const OpenSim::Component&)>& f)
{
    for (const OpenSim::Component& c : component.getComponentList()) {
        f(c);
    }
}

void osc::ForEachComponentInclusive(
    const OpenSim::Component& component,
    const std::function<void(const OpenSim::Component&)>& f)
{
    f(component);
    ForEachComponent(component, f);
}

size_t osc::GetNumChildren(const OpenSim::Component& c)
{
    size_t rv = 0;
    for (const OpenSim::Component& descendant : c.getComponentList()) {
        if (&descendant.getOwner() == &c) {
            ++rv;
        }
    }
    return rv;
}

bool osc::IsInclusiveChildOf(const OpenSim::Component* parent, const OpenSim::Component* c)
{
    if (parent == nullptr) {
        return false;
    }

    for (; c != nullptr; c = GetOwner(*c)) {
        if (c == parent) {
            return true;
        }
    }

    return false;
}

const OpenSim::Component* osc::IsInclusiveChildOf(std::span<const OpenSim::Component*> parents, const OpenSim::Component* c)
{
    // TODO: this method signature makes no sense and should be refactored
    for (; c; c = GetOwner(*c)) {
        if (auto it = rgs::find(parents, c); it != parents.end()) {
            return *it;
        }
    }
    return nullptr;
}

const OpenSim::Component* osc::FindFirstAncestorInclusive(const OpenSim::Component* c, bool(*pred)(const OpenSim::Component*))
{
    for (; c; c = GetOwner(*c)) {
        if (pred(c)) {
            return c;
        }
    }
    return nullptr;
}

const OpenSim::Component* osc::FindFirstDescendentInclusive(
    const OpenSim::Component& component,
    bool(*predicate)(const OpenSim::Component&))
{
    if (predicate(component)) {
        return &component;
    }
    else {
        return FindFirstDescendent(component, predicate);
    }
}

const OpenSim::Component* osc::FindFirstDescendent(
    const OpenSim::Component& component,
    bool(*predicate)(const OpenSim::Component&))
{
    for (const OpenSim::Component& descendent : component.getComponentList()) {
        if (predicate(descendent)) {
            return &descendent;
        }
    }
    return nullptr;
}

std::vector<const OpenSim::Coordinate*> osc::GetCoordinatesInModel(const OpenSim::Model& model)
{
    std::vector<const OpenSim::Coordinate*> rv;
    GetCoordinatesInModel(model, rv);
    return rv;
}

void osc::GetCoordinatesInModel(
    const OpenSim::Model& m,
    std::vector<const OpenSim::Coordinate*>& out)
{
    const OpenSim::CoordinateSet& s = m.getCoordinateSet();
    out.reserve(out.size() + size(s));

    for (size_t i = 0; i < size(s); ++i) {
        out.push_back(&At(s, i));
    }
}

std::vector<OpenSim::Coordinate*> osc::UpdDefaultLockedCoordinatesInModel(OpenSim::Model& model)
{
    std::vector<OpenSim::Coordinate*> rv;
    for (auto& c : model.updComponentList<OpenSim::Coordinate>()) {
        if (c.getDefaultLocked()) {
            rv.push_back(&c);
        }
    }
    return rv;
}


float osc::ConvertCoordValueToDisplayValue(const OpenSim::Coordinate& c, double v)
{
    auto rv = static_cast<float>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational) {
        rv = Degrees{Radians{rv}}.count();
    }

    return rv;
}

double osc::ConvertCoordDisplayValueToStorageValue(const OpenSim::Coordinate& c, float v)
{
    auto rv = static_cast<double>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational) {
        rv = Radians{Degrees{rv}}.count();
    }

    return rv;
}

CStringView osc::GetCoordDisplayValueUnitsString(const OpenSim::Coordinate& c)
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

std::vector<std::string> osc::GetSocketNames(const OpenSim::Component& c)
{
    return c.getSocketNames();
}

std::vector<const OpenSim::AbstractSocket*> osc::GetAllSockets(const OpenSim::Component& c)
{
    std::vector<const OpenSim::AbstractSocket*> rv;

    for (const std::string& name : GetSocketNames(c)) {
        const OpenSim::AbstractSocket& sock = c.getSocket(name);
        rv.push_back(&sock);
    }

    return rv;
}

namespace
{
    enum class GraphEdgeType { ParentChild, Socket };

    struct GraphEdge final {
        friend auto operator<=>(const GraphEdge&, const GraphEdge&) = default;  // NOLINT(hicpp-use-nullptr,modernize-use-nullptr)

        std::string sourceAbsPath;
        std::string destinationAbsPath;
        std::string name;
        GraphEdgeType type = GraphEdgeType::ParentChild;
    };

    void emitGraph(
        const std::set<GraphEdge>& edges,
        std::ostream& out)
    {
        out << "digraph Component {\n";
        for (const auto& edge : edges) {
            out << "    \"" << edge.sourceAbsPath << "\" -> \"" << edge.destinationAbsPath << '"';
            if (edge.type == GraphEdgeType::ParentChild) {
                out << " [color=grey];";
            }
            else if (edge.type == GraphEdgeType::Socket) {
                out << " [label=\"" << edge.name << "\"];";
            }
            out << '\n';
        }
        out << "}";
    }
}

void osc::WriteComponentTopologyGraphAsDotViz(
    const OpenSim::Component& root,
    std::ostream& out)
{
    std::set<GraphEdge> edges;

    // first, get all parent-to-child connections (easiest)
    for (const OpenSim::Component& child : root.getComponentList()) {
        const OpenSim::Component& parent = child.getOwner();

        edges.insert({
            .sourceAbsPath = GetAbsolutePathString(parent),
            .destinationAbsPath = GetAbsolutePathString(child),
            .name = "",
            .type = GraphEdgeType::ParentChild
        });
    }

    // helper: extract all socket edges leaving the given component
    auto extractSocketEdges = [&edges](const OpenSim::Component& c)
    {
        auto sourceAbsPath = GetAbsolutePathString(c);
        for (const OpenSim::AbstractSocket* sock : GetAllSockets(c)) {
            if (const auto* connectee = dynamic_cast<const OpenSim::Component*>(&sock->getConnecteeAsObject())) {
                edges.insert({
                    .sourceAbsPath = sourceAbsPath,
                    .destinationAbsPath = GetAbsolutePathString(*connectee),
                    .name = sock->getName(),
                    .type = GraphEdgeType::Socket,
                });
            }
        }
    };

    extractSocketEdges(root);
    for (const OpenSim::Component& c : root.getComponentList()) {
        extractSocketEdges(c);
    }

    // emit dotviz bitstream
    emitGraph(edges, out);
}

void osc::WriteModelMultibodySystemGraphAsDotViz(
    const OpenSim::Model& model,
    std::ostream& out)
{
    std::set<GraphEdge> edges;
    for (const auto& joint : model.getComponentList<OpenSim::Joint>()) {
        edges.insert(GraphEdge{
            .sourceAbsPath = joint.getChildFrame().findBaseFrame().getAbsolutePathString(),
            .destinationAbsPath = joint.getParentFrame().findBaseFrame().getAbsolutePathString(),
            .name = joint.getAbsolutePathString(),
            .type = GraphEdgeType::Socket,
        });
    }
    emitGraph(edges, out);
}

std::vector<OpenSim::AbstractSocket*> osc::UpdAllSockets(OpenSim::Component& c)
{
    std::vector<OpenSim::AbstractSocket*> rv;

    for (const std::string& name : GetSocketNames(c)) {
        rv.push_back(&c.updSocket(name));
    }

    return rv;
}

const OpenSim::Component* osc::FindComponent(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return FindComponentGeneric(root, cp);
}

const OpenSim::Component* osc::FindComponent(
    const OpenSim::Model& model,
    const std::string& absPath)
{
    return FindComponent(model, OpenSim::ComponentPath{absPath});
}

const OpenSim::Component* osc::FindComponent(
    const OpenSim::Model& model,
    const StringName& absPath)
{
    return FindComponent(model, std::string{absPath});
}

OpenSim::Component* osc::FindComponentMut(
    OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return FindComponentGeneric(root, cp);
}

bool osc::ContainsComponent(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return FindComponent(root, cp) != nullptr;
}

const OpenSim::AbstractSocket* osc::FindSocket(
    const OpenSim::Component& c,
    const std::string& name)
{
    try {
        return &c.getSocket(name);
    }
    catch (const OpenSim::SocketNotFound&) {
        return nullptr;  // :(
    }
}

OpenSim::AbstractSocket* osc::FindSocketMut(
    OpenSim::Component& c,
    const std::string& name)
{
    try {
        return &c.updSocket(name);
    }
    catch (const OpenSim::SocketNotFound&) {
        return nullptr;  // :(
    }
}

bool osc::IsConnectedTo(
    const OpenSim::AbstractSocket& s,
    const OpenSim::Component& c)
{
    return &s.getConnecteeAsObject() == &c;
}

bool osc::IsAbleToConnectTo(
    const OpenSim::AbstractSocket& s,
    const OpenSim::Component& c)
{
    return s.canConnectTo(c);
}

void osc::RecursivelyReassignAllSockets(
    OpenSim::Component& root,
    const OpenSim::Component& from,
    const OpenSim::Component& to)
{
    for (OpenSim::Component& c : root.updComponentList()) {
        for (OpenSim::AbstractSocket* socket : UpdAllSockets(c)) {
            if (IsConnectedTo(*socket, from)) {
                TryConnectTo(*socket, to);
            }
        }
    }
}

OpenSim::AbstractProperty* osc::FindPropertyMut(
    OpenSim::Component& c,
    const std::string& name)
{
    return c.hasProperty(name) ? &c.updPropertyByName(name) : nullptr;
}

const OpenSim::AbstractOutput* osc::FindOutput(
    const OpenSim::Component& c,
    const std::string& outputName)
{
    const OpenSim::AbstractOutput* rv = nullptr;
    try {
        rv = &c.getOutput(outputName);
    }
    catch (const OpenSim::Exception&) {
        // OpenSim, innit :(
    }
    return rv;
}

const OpenSim::AbstractOutput* osc::FindOutput(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& path,
    const std::string& outputName)
{
    const OpenSim::Component* const c = FindComponent(root, path);
    return c ? FindOutput(*c, outputName) : nullptr;
}

bool osc::HasInputFileName(const OpenSim::Model& m)
{
    const std::string& name = m.getInputFileName();
    return !name.empty() && name != "Unassigned";
}

std::optional<std::filesystem::path> osc::TryFindInputFile(const OpenSim::Model& m)
{
    if (not HasInputFileName(m)) {
        return std::nullopt;
    }

    std::filesystem::path p{m.getInputFileName()};
    if (not std::filesystem::exists(p)) {
        return std::nullopt;
    }

    return p;
}

std::optional<std::filesystem::path> osc::FindGeometryFileAbsPath(
    const OpenSim::Model& model,
    const OpenSim::Mesh& mesh)
{
    // this implementation is designed to roughly mimic how OpenSim::Mesh::extendFinalizeFromProperties works

    const std::string& fileProp = mesh.get_mesh_file();
    const std::filesystem::path filePropPath{fileProp};

    bool isAbsolute = filePropPath.is_absolute();
    SimTK::Array_<std::string> attempts;
    const bool found = OpenSim::ModelVisualizer::findGeometryFile(
        model,
        fileProp,
        isAbsolute,
        attempts
    );

    if (!found || attempts.empty()) {
        return std::nullopt;
    }

    return std::optional<std::filesystem::path>{std::filesystem::weakly_canonical({attempts.back()})};
}

std::string osc::GetMeshFileName(const OpenSim::Mesh& mesh)
{
    std::filesystem::path p{mesh.get_mesh_file()};
    return p.filename().string();
}

bool osc::ShouldShowInUI(const OpenSim::Component& c)
{
    if (dynamic_cast<const OpenSim::PathWrapPoint*>(&c)) {
        return false;
    }
    else if (dynamic_cast<const OpenSim::Station*>(&c) && OwnerIs<OpenSim::PathPoint>(c)) {
        return false;
    }
    else {
        return true;
    }
}

bool osc::TryDeleteComponentFromModel(OpenSim::Model& m, OpenSim::Component& c)
{
    OpenSim::Component* const owner = UpdOwner(m, c);

    if (!owner) {
        log_error("cannot delete %s: it has no owner", c.getName().c_str());
        return false;
    }

    if (&c.getRoot() != &m) {
        log_error("cannot delete %s: it is not owned by the provided model", c.getName().c_str());
        return false;
    }

    // check if anything connects to the component as a non-child (i.e. non-hierarchically)
    // via a socket to the component, which may break the other component (so halt deletion)
    if (auto connectees = GetAnyNonChildrenComponentsConnectedViaSocketTo(m, c); !connectees.empty())
    {
        std::stringstream ss;
        CStringView delim;
        for (const OpenSim::Component* connectee : connectees) {
            ss << delim << connectee->getName();
            delim = ", ";
        }
        log_error("cannot delete %s: the following components connect to it via sockets: %s", c.getName().c_str(), std::move(ss).str().c_str());
        return false;
    }

    // HACK: check if any path wraps connect to the component
    //
    // this is because the wrapping code isn't using sockets :< - this should be
    // fixed in OpenSim itself
    for (const OpenSim::PathWrap& pw : m.getComponentList<OpenSim::PathWrap>()) {
        if (pw.getWrapObject() == &c) {
            log_error("cannot delete %s: it is used in a path wrap (%s)", c.getName().c_str(), GetAbsolutePathString(pw).c_str());
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
    if (auto* componentSet = dynamic_cast<OpenSim::ComponentSet*>(owner)) {
        rv = TryDeleteItemFromSet<OpenSim::ModelComponent, OpenSim::ModelComponent>(*componentSet, dynamic_cast<OpenSim::ModelComponent*>(&c));
    }
    else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(owner)) {
        rv = TryDeleteItemFromSet(*bs, dynamic_cast<OpenSim::Body*>(&c));
    }
    else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(owner)) {
        rv = TryDeleteItemFromSet(*wos, dynamic_cast<OpenSim::WrapObject*>(&c));
    }
    else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner)) {
        rv = TryDeleteItemFromSet(*cs, dynamic_cast<OpenSim::Controller*>(&c));
    }
    else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner)) {
        rv = TryDeleteItemFromSet(*conss, dynamic_cast<OpenSim::Constraint*>(&c));
    }
    else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner)) {
        rv = TryDeleteItemFromSet(*fs, dynamic_cast<OpenSim::Force*>(&c));
    }
    else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner)) {
        rv = TryDeleteItemFromSet(*ms, dynamic_cast<OpenSim::Marker*>(&c));
    }
    else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
        rv = TryDeleteItemFromSet(*cgs, dynamic_cast<OpenSim::ContactGeometry*>(&c));
    }
    else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner)) {
        rv = TryDeleteItemFromSet(*ps, dynamic_cast<OpenSim::Probe*>(&c));
    }
    else if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(owner)) {
        if (auto* app = dynamic_cast<OpenSim::AbstractPathPoint*>(&c)) {
            rv = TryDeleteItemFromSet(gp->updPathPointSet(), app);
        }
        else if (auto* pw = dynamic_cast<OpenSim::PathWrap*>(&c)) {
            rv = TryDeleteItemFromSet(gp->updWrapSet(), pw);
        }
    }
    else if (auto* geom = dynamic_cast<OpenSim::Geometry*>(&c)) {
        // delete an OpenSim::Geometry from its owning OpenSim::Frame

        if (auto* frame = dynamic_cast<OpenSim::Frame*>(owner)) {
            // its owner is a frame, which holds the geometry in a list property

            // make a copy of the property containing the geometry and
            // only copy over the not-deleted geometry into the copy
            //
            // this is necessary because OpenSim::Property doesn't seem
            // to support list element deletion, but does support full
            // assignment

            auto& prop = dynamic_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(frame->updProperty_attached_geometry());
            auto copy = Clone(prop);
            copy->clear();

            for (int i = 0; i < prop.size(); ++i) {
                if (OpenSim::Geometry& g = prop[i]; &g != geom) {
                    Append(*copy, g);
                }
            }

            prop.assign(*copy);

            rv = true;
        }
    }

    if (!rv) {
        log_error("cannot delete %s: OpenSim Creator doesn't know how to delete a %s from its parent (maybe it can't?)", c.getName().c_str(), c.getConcreteClassName().c_str());
    }

    return rv;
}

void osc::CopyCommonJointProperties(const OpenSim::Joint& src, OpenSim::Joint& dest)
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
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (size_t i = 0; i < size(wos); ++i) {
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
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (size_t i = 0; i < size(wos); ++i) {
            OpenSim::WrapObject& wo = At(wos, i);
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
            rv = true;
        }
    }
    return rv;
}

std::vector<const OpenSim::WrapObject*> osc::GetAllWrapObjectsReferencedBy(const OpenSim::GeometryPath& gp)
{
    const auto& wrapSet = gp.getWrapSet();

    std::vector<const OpenSim::WrapObject*> rv;
    rv.reserve(wrapSet.getSize());
    for (int i = 0; i < wrapSet.getSize(); ++i) {
        rv.push_back(wrapSet.get(i).getWrapObject());
    }
    return rv;
}

void osc::InitializeModel(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeModel");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)
    model.clearConnections();        // clears any potentially stale pointers that can be retained by OpenSim::Socket<T> (see #263)
    model.buildSystem();             // creates a new underlying physics system
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

std::optional<size_t> osc::FindJointInParentJointSet(const OpenSim::Joint& joint)
{
    const auto* parentJointset = GetOwner<OpenSim::JointSet>(joint);
    if (not parentJointset) {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return std::nullopt;
    }

    return IndexOf(*parentJointset, joint);
}

std::string osc::GetDisplayName(const OpenSim::Geometry& g)
{
    if (const auto* mesh = dynamic_cast<const OpenSim::Mesh*>(&g); mesh) {
        return mesh->getGeometryFilename();
    }
    else {
        return g.getConcreteClassName();
    }
}

CStringView osc::GetMotionTypeDisplayName(const OpenSim::Coordinate& c)
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

const OpenSim::Appearance* osc::TryGetAppearance(const OpenSim::Component& component)
{
    if (!component.hasProperty("Appearance")) {
        return nullptr;
    }

    const OpenSim::AbstractProperty& abstractProperty = component.getPropertyByName("Appearance");
    const auto* maybeAppearanceProperty = dynamic_cast<const OpenSim::Property<OpenSim::Appearance>*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->getValue() : nullptr;
}

OpenSim::Appearance* osc::TryUpdAppearance(OpenSim::Component& component)
{
    if (!component.hasProperty("Appearance")) {
        return nullptr;
    }

    OpenSim::AbstractProperty& abstractProperty = component.updPropertyByName("Appearance");
    auto* maybeAppearanceProperty = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->updValue() : nullptr;
}

bool osc::TrySetAppearancePropertyIsVisibleTo(OpenSim::Component& c, bool v)
{
    if (OpenSim::Appearance* appearance = TryUpdAppearance(c)) {
        appearance->set_visible(v);
        return true;
    }
    else {
        return false;
    }
}

Color osc::to_color(const OpenSim::Appearance& appearance)
{
    const SimTK::Vec3& rgb = appearance.get_color();
    const double a = appearance.get_opacity();

    return {
        static_cast<float>(rgb[0]),
        static_cast<float>(rgb[1]),
        static_cast<float>(rgb[2]),
        static_cast<float>(a),
    };
}

Color osc::GetSuggestedBoneColor()
{
    Color usualDefault = {232.0f / 255.0f, 216.0f / 255.0f, 200.0f/255.0f, 1.0f};
    float brightenAmount = 0.1f;
    return lerp(usualDefault, Color::white(), brightenAmount);
}

bool osc::IsShowingFrames(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_frames();
}

bool osc::ToggleShowingFrames(OpenSim::Model& model)
{
    const bool newValue = !IsShowingFrames(model);
    model.updDisplayHints().set_show_frames(newValue);
    return newValue;
}

bool osc::IsShowingMarkers(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_markers();
}

bool osc::ToggleShowingMarkers(OpenSim::Model& model)
{
    const bool newValue = !IsShowingMarkers(model);
    model.updDisplayHints().set_show_markers(newValue);
    return newValue;
}

bool osc::IsShowingWrapGeometry(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_wrap_geometry();
}

bool osc::ToggleShowingWrapGeometry(OpenSim::Model& model)
{
    const bool newValue = !IsShowingWrapGeometry(model);
    model.updDisplayHints().set_show_wrap_geometry(newValue);
    return newValue;
}

bool osc::IsShowingContactGeometry(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_contact_geometry();
}

bool osc::IsShowingForces(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_forces();
}

bool osc::ToggleShowingContactGeometry(OpenSim::Model& model)
{
    const bool newValue = !IsShowingContactGeometry(model);
    model.updDisplayHints().set_show_contact_geometry(newValue);
    return newValue;
}

bool osc::ToggleShowingForces(OpenSim::Model& model)
{
    const bool newValue = !IsShowingForces(model);
    model.updDisplayHints().set_show_forces(newValue);
    return newValue;
}

void osc::GetAbsolutePathString(const OpenSim::Component& c, std::string& out)
{
    constexpr ptrdiff_t c_MaxEls = 16;

    ptrdiff_t nEls = 0;
    std::array<const OpenSim::Component*, c_MaxEls> els{};
    const OpenSim::Component* cur = &c;
    const OpenSim::Component* next = GetOwner(*cur);

    if (!next) {
        // edge-case: caller provided a root
        out = '/';
        return;
    }

    while (cur && next && nEls < c_MaxEls) {
        els[nEls++] = cur;
        cur = next;
        next = GetOwner(*cur);
    }

    if (nEls >= c_MaxEls) {
        // edge-case: component is too deep: fallback to OpenSim impl.
        out = c.getAbsolutePathString();
        return;
    }

    // else: construct the path piece-by-piece

    // precompute path length (memory allocation)
    size_t pathlen = nEls;
    for (ptrdiff_t i = 0; i < nEls; ++i) {
        pathlen += els[i]->getName().size();
    }

    // then preallocate the string
    out.resize(pathlen);

    // and assign it
    size_t loc = 0;
    for (ptrdiff_t i = nEls-1; i >= 0; --i) {
        out[loc++] = '/';
        const std::string& name = els[i]->getName();
        rgs::copy(name, out.begin() + loc);
        loc += name.size();
    }
}

std::string osc::GetAbsolutePathString(const OpenSim::Component& c)
{
    std::string rv;
    GetAbsolutePathString(c, rv);
    return rv;
}

StringName osc::GetAbsolutePathStringName(const OpenSim::Component& c)
{
    return StringName{GetAbsolutePathString(c)};
}

OpenSim::ComponentPath osc::GetAbsolutePath(const OpenSim::Component& c)
{
    return OpenSim::ComponentPath{GetAbsolutePathString(c)};
}

OpenSim::ComponentPath osc::GetAbsolutePathOrEmpty(const OpenSim::Component* c)
{
    if (c) {
        return GetAbsolutePath(*c);
    }
    else {
        return OpenSim::ComponentPath{};
    }
}

std::optional<LinesOfAction> osc::GetEffectiveLinesOfActionInGround(
    const OpenSim::Muscle& muscle,
    const SimTK::State& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = true;
    return TryGetLinesOfAction(muscle, state, config);
}

std::optional<LinesOfAction> osc::GetAnatomicalLinesOfActionInGround(
    const OpenSim::Muscle& muscle,
    const SimTK::State& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = false;
    return TryGetLinesOfAction(muscle, state, config);
}

std::vector<std::unique_ptr<OpenSim::PointForceDirection>> osc::GetPointForceDirections(
    const OpenSim::GeometryPath& path,
    const SimTK::State& st)
{
    OpenSim::Array<OpenSim::PointForceDirection*> pfds;
    path.getPointForceDirections(st, &pfds);

    std::vector<std::unique_ptr<OpenSim::PointForceDirection>> rv;
    rv.reserve(size(pfds));
    for (size_t i = 0; i < size(pfds); ++i) {
        rv.emplace_back(At(pfds, i));
    }
    return rv;
}

std::vector<GeometryPathPoint> osc::GetAllPathPoints(const OpenSim::GeometryPath& gp, const SimTK::State& st)
{
    const OpenSim::Array<OpenSim::AbstractPathPoint*>& pps = gp.getCurrentPath(st);

    std::vector<GeometryPathPoint> rv;
    rv.reserve(size(pps));  // best guess: but path wrapping might add more

    for (size_t i = 0; i < size(pps); ++i) {
        const OpenSim::AbstractPathPoint* const ap = At(pps, i);

        if (ap == nullptr) {
            // defensive case: there's `nullptr` in the pointset, ignore it
            continue;
        }
        else if (const auto* pwp = dynamic_cast<const OpenSim::PathWrapPoint*>(ap)) {
            // special case: it's a wrapping point, so add each part of the wrap
            const auto body2ground = to<Transform>(pwp->getParentFrame().getTransformInGround(st));
            const OpenSim::Array<SimTK::Vec3>& wrapPath = pwp->getWrapPath(st);

            rv.reserve(rv.size() + size(wrapPath));
            for (size_t j = 0; j < size(wrapPath); ++j) {
                rv.emplace_back(body2ground * to<Vec3>(At(wrapPath, j)));
            }
        }
        else {
            // typical case: it's a normal/computed point with a single location in ground
            rv.emplace_back(*ap, to<Vec3>(ap->getLocationInGround(st)));
        }
    }

    return rv;
}

namespace
{
    // returns the first ContactHalfSpace found within the given HuntCrossleyForce's parameters, or
    // nullptr, if no ContactHalfSpace could be found
    const OpenSim::ContactHalfSpace* TryFindFirstContactHalfSpace(
        const OpenSim::Model& model,
        const OpenSim::HuntCrossleyForce& hcf)
    {
        // get contact parameters (i.e. where the contact geometry is stored)
        const OpenSim::HuntCrossleyForce::ContactParametersSet& paramSet = hcf.get_contact_parameters();
        if (empty(paramSet)) {
            return nullptr;  // edge-case: the force has no parameters
        }

        // linearly search for a ContactHalfSpace
        for (size_t i = 0; i < size(paramSet); ++i) {
            const OpenSim::HuntCrossleyForce::ContactParameters& param = At(paramSet, i);
            const OpenSim::Property<std::string>& geomProperty = param.getProperty_geometry();

            for (size_t j = 0; j < size(geomProperty); ++j) {
                const std::string& geomNameOrPath = At(geomProperty, j);
                if (const auto* foundViaAbsPath = FindComponent<OpenSim::ContactHalfSpace>(model, geomNameOrPath)) {
                    // found it as an abspath within the model
                    return foundViaAbsPath;
                }
                else if (const auto* foundViaRelativePath = FindComponent<OpenSim::ContactHalfSpace>(model.getContactGeometrySet(), geomNameOrPath)) {
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
        const OpenSim::HuntCrossleyForce& hcf,
        const SimTK::State& state)
    {
        const OpenSim::Array<double> forces = hcf.getRecordValues(state);
        if (forces.size() < 6) {
            return std::nullopt;  // edge-case: OpenSim didn't report the expected forces
        }

        const Vec3 force{
            static_cast<float>(-forces[0]),
            static_cast<float>(-forces[1]),
            static_cast<float>(-forces[2]),
        };

        if (length2(force) < epsilon_v<float>) {
            return std::nullopt;  // edge-case: no force is actually being exherted
        }

        const Vec3 torque{
            static_cast<float>(-forces[3]),
            static_cast<float>(-forces[4]),
            static_cast<float>(-forces[5]),
        };

        return ForceTorque{force, torque};
    }

    // helper: convert an OpenSim::ContactHalfSpace, which is defined in a frame with an offset,
    //         etc. into a simpler "plane in groundspace" representation that's more useful
    //         for rendering
    Plane ToAnalyticPlaneInGround(
        const OpenSim::ContactHalfSpace& halfSpace,
        const SimTK::State& state)
    {
        // go through the contact geometries that are attached to the force
        //
        // - if there's a plane, then the plane's location+normal are needed in order
        //   to figure out where the force is exherted
        const auto body2ground = to<Transform>(halfSpace.getFrame().getTransformInGround(state));
        const auto geom2body = to<Transform>(halfSpace.getTransform());

        const Vec3 originInGround = body2ground * to<Vec3>(halfSpace.get_location());
        const Vec3 normalInGround = normalize(body2ground.rotation * geom2body.rotation) * c_ContactHalfSpaceUpwardsNormal;

        return Plane{originInGround, normalInGround};
    }

    // helper: returns the location of the center of pressure of a force+torque on a plane, or
    //         std::nullopt if the to-be-drawn force vector is too small
    std::optional<Vec3> ComputeCenterOfPressure(
        const Plane& plane,
        const ForceTorque& forceTorque,
        float minimumForce = epsilon_v<float>)
    {
        const float forceScaler = dot(plane.normal, forceTorque.force);

        if (abs(forceScaler) < minimumForce) {
            // edge-case: the resulting force vector is too small
            return std::nullopt;
        }

        if (abs(dot(plane.normal, normalize(forceTorque.torque))) >= 1.0f - epsilon_v<float>) {
            // pedantic: the resulting torque is aligned with the plane normal, making
            // the cross product undefined later
            return std::nullopt;
        }

        // this maths seems sketchy, it's inspired by SCONE/model_tools.cpp:GetPlaneCop but
        // it feels a bit like `p1` is always going to be zero
        const Vec3 pos = cross(plane.normal, forceTorque.torque) / forceScaler;
        const Vec3 posRelativeToPlaneOrigin = pos - plane.origin;
        const float p1 = dot(posRelativeToPlaneOrigin, plane.normal);
        const float p2 = forceScaler;

        return pos - (p1/p2)*forceTorque.force;
    }
}

std::optional<ForcePoint> osc::TryGetContactForceInGround(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::HuntCrossleyForce& hcf)
{
    // try and find a contact half space to attach the force vectors to
    const OpenSim::ContactHalfSpace* const maybeContactHalfSpace = TryFindFirstContactHalfSpace(model, hcf);
    if (!maybeContactHalfSpace) {
        return std::nullopt;  // couldn't find a ContactHalfSpace
    }
    const Plane contactPlaneInGround = ToAnalyticPlaneInGround(*maybeContactHalfSpace, state);

    // try and compute the force vectors
    const std::optional<ForceTorque> maybeForceTorque = TryComputeCurrentForceTorque(hcf, state);
    if (!maybeForceTorque) {
        return std::nullopt;  // couldn't extract force+torque from the HCF
    }
    const ForceTorque& forceTorque = *maybeForceTorque;

    const std::optional<Vec3> maybePosition = ComputeCenterOfPressure(contactPlaneInGround, forceTorque);
    if (!maybePosition) {
        return std::nullopt;  // the resulting force is too small
    }

    return ForcePoint{forceTorque.force, *maybePosition};
}

const OpenSim::PhysicalFrame& osc::GetFrameUsingExternalForceLookupHeuristic(
    const OpenSim::Model& model,
    const std::string& bodyNameOrPath)
{
    // this tries to match the implementation that's hidden inside
    // of `ExternalForce.cpp` from OpenSim

    if (const auto* direct = FindComponent<OpenSim::PhysicalFrame>(model, bodyNameOrPath)) {
        return *direct;
    }
    else if (const auto* shimmed = FindComponent<OpenSim::PhysicalFrame>(model, "./bodyset/" + bodyNameOrPath)) {
        return *shimmed;
    }
    else {
        return model.getGround();
    }
}

bool osc::CanExtractPointInfoFrom(const OpenSim::Component& c, const SimTK::State& st)
{
    return TryExtractPointInfo(c, st) != std::nullopt;
}

std::optional<PointInfo> osc::TryExtractPointInfo(
    const OpenSim::Component& c,
    const SimTK::State& st)
{
    if (dynamic_cast<const OpenSim::PathWrapPoint*>(&c)) {
        // HACK: path wrap points don't update the cache correctly?
        return std::nullopt;
    }
    else if (const auto* station = dynamic_cast<const OpenSim::Station*>(&c)) {
        // HACK: OpenSim redundantly stores path point information in a child called 'station'.
        // These must be filtered because, otherwise, the user will just see a bunch of
        // 'station' entries below each path point
        if (station->getName() == "station" && OwnerIs<OpenSim::PathPoint>(*station)) {
            return std::nullopt;
        }

        return PointInfo{
            to<Vec3>(station->get_location()),
            GetAbsolutePath(station->getParentFrame()),
        };
    }
    else if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&c)) {
        return PointInfo{
            to<Vec3>(pp->getLocation(st)),
            GetAbsolutePath(pp->getParentFrame()),
        };
    }
    else if (const auto* point = dynamic_cast<const OpenSim::Point*>(&c)) {
        return PointInfo{
            to<Vec3>(point->getLocationInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }
    else if (const auto* frame = dynamic_cast<const OpenSim::Frame*>(&c)) {
        return PointInfo{
            to<Vec3>(frame->getPositionInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }
    else {
        return std::nullopt;
    }
}

OpenSim::Component& osc::AddComponentToAppropriateSet(OpenSim::Model& m, std::unique_ptr<OpenSim::Component> c)
{
    if (c == nullptr) {
        throw std::runtime_error{"nullptr passed to AddComponentToAppropriateSet"};
    }

    OpenSim::Component& rv = *c;

    if (dynamic_cast<OpenSim::Body*>(c.get())) {
        m.addBody(dynamic_cast<OpenSim::Body*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Joint*>(c.get())) {
        m.addJoint(dynamic_cast<OpenSim::Joint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Constraint*>(c.get())) {
        m.addConstraint(dynamic_cast<OpenSim::Constraint*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Force*>(c.get())) {
        m.addForce(dynamic_cast<OpenSim::Force*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Probe*>(c.get())) {
        m.addProbe(dynamic_cast<OpenSim::Probe*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::ContactGeometry*>(c.get())) {
        m.addContactGeometry(dynamic_cast<OpenSim::ContactGeometry*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Marker*>(c.get())) {
        m.addMarker(dynamic_cast<OpenSim::Marker*>(c.release()));
    }
    else if (dynamic_cast<OpenSim::Controller*>(c.get())) {
        m.addController(dynamic_cast<OpenSim::Controller*>(c.release()));
    }
    else {
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

OpenSim::WrapObject& osc::AddWrapObject(OpenSim::PhysicalFrame& physFrame, std::unique_ptr<OpenSim::WrapObject> wrapObj)
{
    OpenSim::WrapObject& rv = *wrapObj;
    physFrame.addWrapObject(wrapObj.release());
    return rv;
}

OpenSim::Geometry& osc::AttachGeometry(OpenSim::Frame& frame, std::unique_ptr<OpenSim::Geometry> p)
{
    OpenSim::Geometry& rv = *p;
    frame.attachGeometry(p.release());
    return rv;
}

const OpenSim::PhysicalFrame* osc::TryGetParentToGroundFrame(const OpenSim::Component& component)
{
    if (const auto* station = dynamic_cast<const OpenSim::Station*>(&component)) {
        return &station->getParentFrame();
    }
    else if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&component)) {
        return &pp->getParentFrame();
    }
    else if (const auto* pof = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&component)) {
        return &pof->getParentFrame();
    }
    else {
        return nullptr;
    }
}

std::optional<SimTK::Transform> osc::TryGetParentToGroundTransform(
    const OpenSim::Component& component,
    const SimTK::State& state)
{
    if (const OpenSim::PhysicalFrame* frame = TryGetParentToGroundFrame(component)) {
        return frame->getTransformInGround(state);
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::string> osc::TryGetPositionalPropertyName(
    const OpenSim::Component& component)
{
    if (const auto* station = dynamic_cast<const OpenSim::Station*>(&component)) {
        return station->getProperty_location().getName();
    }
    else if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&component)) {
        return pp->getProperty_location().getName();
    }
    else if (const auto* pof = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&component)) {
        return pof->getProperty_translation().getName();
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::string> osc::TryGetOrientationalPropertyName(
    const OpenSim::Component& component)
{
    if (const auto* pof = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&component)) {
        return pof->getProperty_orientation().getName();
    }
    else {
        return std::nullopt;
    }
}

const OpenSim::Frame* osc::TryGetParentFrame(const OpenSim::Frame& frame)
{
    if (auto offset = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&frame)) {
        return &offset->getParentFrame();
    }
    return nullptr;
}

std::optional<ComponentSpatialRepresentation> osc::TryGetSpatialRepresentation(
    const OpenSim::Component& component,
    const SimTK::State& state)
{
    if (auto xform = TryGetParentToGroundTransform(component, state)) {
        if (auto posProp = TryGetPositionalPropertyName(component)) {
            return ComponentSpatialRepresentation{
                *xform,
                std::move(posProp).value(),
                TryGetOrientationalPropertyName(component)
            };
        }
    }
    return std::nullopt;
}

bool osc::IsValidOpenSimComponentNameCharacter(char c)
{
    return
        std::isalpha(static_cast<uint8_t>(c)) != 0 ||
        ('0' <= c && c <= '9') ||
        (c == '-' || c == '_');
}

std::string osc::SanitizeToOpenSimComponentName(std::string_view sv)
{
    std::string rv;
    for (auto c : sv) {
        if (IsValidOpenSimComponentNameCharacter(c)) {
            rv += c;
        }
    }
    return rv;
}

std::unique_ptr<OpenSim::Storage> osc::LoadStorage(
    const OpenSim::Model& model,
    const std::filesystem::path& path,
    const StorageLoadingParameters& params)
{
    auto rv = std::make_unique<OpenSim::Storage>(path.string());

    if (params.convertRotationalValuesToRadians and rv->isInDegrees()) {
        model.getSimbodyEngine().convertDegreesToRadians(*rv);
    }

    if (params.resampleToFrequency) {
        rv->resampleLinear(*params.resampleToFrequency);
    }

    return rv;
}

std::unordered_map<int, int> osc::CreateStorageIndexToModelStatevarMappingWithWarnings(
    const OpenSim::Model& model,
    const OpenSim::Storage& storage)
{
    auto mapping = CreateStorageIndexToModelStatevarMapping(model, storage);
    if (not mapping.stateVariablesMissingInStorage.empty()) {
        std::stringstream ss;
        ss << "the provided STO file is missing the following columns:\n";
        std::string_view delim;
        for (const std::string& el : mapping.stateVariablesMissingInStorage) {
            ss << delim << el;
            delim = ", ";
        }
        log_warn("%s", std::move(ss).str().c_str());
        log_warn("The STO file was loaded successfully, but beware: the missing state variables have been defaulted in order for this to work");
        log_warn("Therefore, do not treat the motion you are seeing as a 'true' representation of something: some state data was 'made up' to make the motion viewable");
    }
    return std::move(mapping.storageIndexToModelStatevarIndex);
}

StorageIndexToModelStateVarMappingResult osc::CreateStorageIndexToModelStatevarMapping(
    const OpenSim::Model& model,
    const OpenSim::Storage& storage)
{
    // ensure the `OpenSim::Storage` holds a time sequence.
    if (not is_equal_case_insensitive(storage.getColumnLabels()[0], "time")) {
        throw std::runtime_error{"the provided motion data does not contain a 'time' column as its first column: it cannot be processed"};
    }

    // get+validate column headers from the `OpenSim::Storage`.
    const OpenSim::Array<std::string>& storageColumnsIncludingTime = storage.getColumnLabels();
    if (not IsAllElementsUnique(storageColumnsIncludingTime)) {
        throw std::runtime_error{"the provided motion data contains multiple columns with the same name. This creates ambiguities that OpenSim Creator can't handle"};
    }

    // get the state variable labels from the `OpenSim::Model`
    OpenSim::Array<std::string> modelStateVars = model.getStateVariableNames();

    StorageIndexToModelStateVarMappingResult rv;
    rv.storageIndexToModelStatevarIndex.reserve(modelStateVars.size());

    // compute storage-to-model index mapping
    //
    // care: The storage's column labels do not match the model's state variable names
    //       1:1. STO files have changed over time. OpenSim pre-4.0 used different naming
    //       conventions for the column labels, so you *need* to map the storage column
    //       strings carefully onto the model statevars.
    for (int modelIndex = 0; modelIndex < modelStateVars.size(); ++modelIndex) {
        const std::string& modelStateVarname = modelStateVars[modelIndex];
        const int storageIndex = OpenSim::TableUtilities::findStateLabelIndex(storageColumnsIncludingTime, modelStateVarname);
        const int valueIndex = storageIndex - 1;  // the column labels include 'time', which isn't in the data elements

        if (valueIndex >= 0) {
            rv.storageIndexToModelStatevarIndex[valueIndex] = modelIndex;
        }
        else {
            rv.stateVariablesMissingInStorage.push_back(modelStateVarname);
        }
    }

    return rv;
}

void osc::UpdateStateVariablesFromStorageRow(
    OpenSim::Model& model,
    SimTK::State& state,
    const std::unordered_map<int, int>& columnIndexToModelStateVarIndex,
    const OpenSim::Storage& storage,
    int row)
{
    // grab the state vector from the `OpenSim::Storage`
    OpenSim::StateVector* sv = storage.getStateVector(row);
    const OpenSim::Array<double>& cols = sv->getData();

    // copy + update the `OpenSim::Model`'s state vector with state variables from the `OpenSim::Storage`
    SimTK::Vector stateValsBuf = model.getStateVariableValues(state);
    for (auto [valueIdx, modelIdx] : columnIndexToModelStateVarIndex) {
        if (0 <= valueIdx && valueIdx < cols.size() && 0 <= modelIdx && modelIdx < stateValsBuf.size()) {
            stateValsBuf[modelIdx] = cols[valueIdx];
        }
        else {
            throw std::runtime_error{"an index in the stroage lookup was invalid: this is probably a developer error that needs to be investigated (report it)"};
        }
    }

    // update state with new state variables and re-assemble, re-realize, etc.
    state.setTime(sv->getTime());
    for (auto& coordinate : model.getComponentList<OpenSim::Coordinate>()) {
        coordinate.setLocked(state, false);
    }
    model.setStateVariableValues(state, stateValsBuf);
}

void osc::UpdateStateFromStorageTime(
    OpenSim::Model& model,
    SimTK::State& state,
    const std::unordered_map<int, int>& columnIndexToModelStateVarIndex,
    const OpenSim::Storage& storage,
    double time)
{
    return UpdateStateVariablesFromStorageRow(model, state, columnIndexToModelStateVarIndex, storage, storage.findIndex(time));
}
