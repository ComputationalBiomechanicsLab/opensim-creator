#include "open_sim_helpers.h"

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/opynsim.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/perf.h>
#include <liboscar/utilities/string_helpers.h>
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
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/ForceConsumer.h>
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
#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/Wrap/PathWrap.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <span>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    constexpr osc::Vector3 c_ContactHalfSpaceUpwardsNormal = {-1.0f, 0.0f, 0.0f};
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
                is_inclusive_child_of(&component, connectee) &&
                GetAnyComponentsConnectedViaSocketTo(root, *connectee).empty();  // care: the child may, itself, have things connected to it
        });
        return allConnectees;
    }

    struct PointForceV2 final {
        osc::Vector3 locationInGround(const SimTK::State& state) const
        {
            return osc::to<osc::Vector3>(frame->findStationLocationInGround(state, point));
        }

        const OpenSim::PhysicalFrame* frame;
        SimTK::Vec3 point{0.0};
        SimTK::Vec3 force{0.0};
    };

    // returns the index of the "effective" origin point of a muscle PFD sequence
    ptrdiff_t GetEffectiveOrigin(std::span<const PointForceV2> pfds)
    {
        OSC_ASSERT_ALWAYS(not pfds.empty());

        // move forward through the PFD sequence until a different frame is found
        //
        // the PFD before that one is the effective origin
        const auto it = find_if(
            pfds.begin() + 1,
            pfds.end(),
            [&first = pfds.front()](const auto& pfd) { return pfd.frame != first.frame; }
        );
        return std::distance(pfds.begin(), it) - 1;
    }

    // returns the index of the "effective" insertion point of a muscle PFD sequence
    ptrdiff_t GetEffectiveInsertion(std::span<const PointForceV2> pfds)
    {
        OSC_ASSERT_ALWAYS(not pfds.empty());

        // move backward through the PFD sequence until a different frame is found
        //
        // the PFD after that one is the effective insertion
        const auto rit = find_if(
            pfds.rbegin() + 1,
            pfds.rend(),
            [&last = pfds.back()](const auto& pfd) { return pfd.frame != last.frame; }
        );
        return std::distance(pfds.begin(), rit.base());
    }

    // returns an index range into the provided array that contains only
    // effective attachment points? (see: https://github.com/modenaxe/MuscleForceDirection/blob/master/CPP/MuscleForceDirection/MuscleForceDirection.cpp)
    std::pair<ptrdiff_t, ptrdiff_t> GetEffectiveAttachmentIndices(std::span<const PointForceV2> pfds)
    {
        return {GetEffectiveOrigin(pfds), GetEffectiveInsertion(pfds)};
    }

    std::pair<ptrdiff_t, ptrdiff_t> GetAnatomicalAttachmentIndices(std::span<const PointForceV2> pfds)
    {
        OSC_ASSERT(!pfds.empty());

        return {0, pfds.size() - 1};
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
        class VectorForceConsumer final : public OpenSim::ForceConsumer {
        public:
            const std::vector<PointForceV2>& forces() const { return accumulator_; }
        private:
            void implConsumePointForce(
                const SimTK::State&,
                const OpenSim::PhysicalFrame& frame,
                const SimTK::Vec3& point,
                const SimTK::Vec3& force) override
            {
                accumulator_.push_back({ .frame = &frame, .point = point, .force = force });
            }

            std::vector<PointForceV2> accumulator_;
        };

        VectorForceConsumer consumer;
        muscle.produceForces(st, consumer);

        if (consumer.forces().size() < 2) {
            return std::nullopt;  // not enough PFDs to compute a line of action
        }

        const std::pair<ptrdiff_t, ptrdiff_t> attachmentIndexRange = config.useEffectiveInsertion ?
            GetEffectiveAttachmentIndices(consumer.forces()) :
            GetAnatomicalAttachmentIndices(consumer.forces());

        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.first && attachmentIndexRange.first < std::ssize(consumer.forces()));
        OSC_ASSERT_ALWAYS(0 <= attachmentIndexRange.second && attachmentIndexRange.second < std::ssize(consumer.forces()));

        if (attachmentIndexRange.first >= attachmentIndexRange.second) {
            return std::nullopt;  // not enough *unique* PFDs to compute a line of action
        }

        const osc::Vector3 originPos = consumer.forces().at(attachmentIndexRange.first).locationInGround(st);
        const osc::Vector3 pointAfterOriginPos = consumer.forces().at(attachmentIndexRange.first + 1).locationInGround(st);
        const osc::Vector3 originDir = normalize(pointAfterOriginPos - originPos);

        const osc::Vector3 insertionPos = consumer.forces().at(attachmentIndexRange.second).locationInGround(st);
        const osc::Vector3 pointAfterInsertionPos = consumer.forces().at(attachmentIndexRange.second - 1).locationInGround(st);
        const osc::Vector3 insertionDir = osc::normalize(pointAfterInsertionPos - insertionPos);

        return LinesOfAction{
            osc::Ray{originPos, originDir},
            osc::Ray{insertionPos, insertionDir},
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

bool opyn::is_concrete_class_name_lexicographically_lower_than(const OpenSim::Component& a, const OpenSim::Component& b)
{
    return a.getConcreteClassName() < b.getConcreteClassName();
}

bool opyn::is_name_lexicographically_lower_than(const OpenSim::Component& a, const OpenSim::Component& b)
{
    return a.getName() < b.getName();
}

OpenSim::Component* opyn::upd_owner(OpenSim::Component& root, const OpenSim::Component& c)
{
    if (const auto* constOwner = get_owner(c)) {
        return find_component_mut(root, get_absolute_path(*constOwner));
    }
    else {
        return nullptr;
    }
}

OpenSim::Component& opyn::upd_owner_or_throw(OpenSim::Component& root, const OpenSim::Component& c)
{
    auto* p = upd_owner(root, c);
    if (not p) {
        throw std::runtime_error{"could not update a component's owner"};
    }
    return *p;
}

const OpenSim::Component& opyn::get_owner_or_throw(const OpenSim::AbstractOutput& ao)
{
    return ao.getOwner();
}

const OpenSim::Component& opyn::get_owner_or_throw(const OpenSim::Component& c)
{
    return c.getOwner();
}

const OpenSim::Component& opyn::get_owner_or(const OpenSim::Component& c, const OpenSim::Component& fallback)
{
    return c.hasOwner() ? c.getOwner() : fallback;
}

const OpenSim::Component* opyn::get_owner(const OpenSim::Component& c)
{
    return c.hasOwner() ? &c.getOwner() : nullptr;
}

std::optional<std::string> opyn::try_get_owner_name(const OpenSim::Component& c)
{
    const OpenSim::Component* owner = get_owner(c);
    return owner ? owner->getName() : std::optional<std::string>{};
}

size_t opyn::distance_from_root(const OpenSim::Component& c)
{
    size_t dist = 0;
    for (const OpenSim::Component* p = &c; p; p = get_owner(*p)) {
        ++dist;
    }
    return dist;
}

OpenSim::ComponentPath opyn::get_root_component_path()
{
    return OpenSim::ComponentPath{"/"};
}

bool opyn::is_empty(const OpenSim::ComponentPath& cp)
{
    return cp == OpenSim::ComponentPath{};
}

void opyn::clear(OpenSim::ComponentPath& cp)
{
    cp = OpenSim::ComponentPath{};
}

std::vector<const OpenSim::Component*> opyn::get_path_elements(const OpenSim::Component& c)
{
    std::vector<const OpenSim::Component*> rv;
    rv.reserve(distance_from_root(c));

    for (const OpenSim::Component* p = &c; p; p = get_owner(*p)) {
        rv.push_back(p);
    }

    rgs::reverse(rv);

    return rv;
}

void opyn::for_each_component(
    const OpenSim::Component& component,
    const std::function<void(const OpenSim::Component&)>& f)
{
    for (const OpenSim::Component& c : component.getComponentList()) {
        f(c);
    }
}

void opyn::for_each_component_inclusive(
    const OpenSim::Component& component,
    const std::function<void(const OpenSim::Component&)>& f)
{
    f(component);
    for_each_component(component, f);
}

size_t opyn::get_num_children(const OpenSim::Component& c)
{
    size_t rv = 0;
    for (const OpenSim::Component& descendant : c.getComponentList()) {
        if (&descendant.getOwner() == &c) {
            ++rv;
        }
    }
    return rv;
}

bool opyn::is_inclusive_child_of(const OpenSim::Component* parent, const OpenSim::Component* c)
{
    if (parent == nullptr) {
        return false;
    }

    for (; c != nullptr; c = get_owner(*c)) {
        if (c == parent) {
            return true;
        }
    }

    return false;
}

const OpenSim::Component* opyn::is_inclusive_child_of(std::span<const OpenSim::Component*> parents, const OpenSim::Component* c)
{
    for (; c; c = get_owner(*c)) {
        if (auto it = rgs::find(parents, c); it != parents.end()) {
            return *it;
        }
    }
    return nullptr;
}

const OpenSim::Component* opyn::find_first_ancestor_inclusive(const OpenSim::Component* c, bool(*pred)(const OpenSim::Component*))
{
    for (; c; c = get_owner(*c)) {
        if (pred(c)) {
            return c;
        }
    }
    return nullptr;
}

const OpenSim::Component* opyn::find_first_descendent_inclusive(
    const OpenSim::Component& component,
    bool(*predicate)(const OpenSim::Component&))
{
    if (predicate(component)) {
        return &component;
    }
    else {
        return find_first_descendent(component, predicate);
    }
}

const OpenSim::Component* opyn::find_first_descendent(
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

OpenSim::Component* opyn::find_first_descendent_mut(
    OpenSim::Component& component,
    bool(*predicate)(const OpenSim::Component&))
{
    for (OpenSim::Component& descendent : component.updComponentList()) {
        if (predicate(descendent)) {
            return &descendent;
        }
    }
    return nullptr;
}

std::vector<const OpenSim::Coordinate*> opyn::get_coordinates_in_model(const OpenSim::Model& model)
{
    std::vector<const OpenSim::Coordinate*> rv;
    get_coordinates_in_model(model, rv);
    return rv;
}

void opyn::get_coordinates_in_model(
    const OpenSim::Model& m,
    std::vector<const OpenSim::Coordinate*>& out)
{
    const OpenSim::CoordinateSet& s = m.getCoordinateSet();
    out.reserve(out.size() + size(s));

    for (size_t i = 0; i < size(s); ++i) {
        out.push_back(&at(s, i));
    }
}

std::vector<OpenSim::Coordinate*> opyn::upd_default_locked_coordinates_in_model(OpenSim::Model& model)
{
    std::vector<OpenSim::Coordinate*> rv;
    for (auto& c : model.updComponentList<OpenSim::Coordinate>()) {
        if (c.getDefaultLocked()) {
            rv.push_back(&c);
        }
    }
    return rv;
}


float opyn::convert_coord_value_to_display_value(const OpenSim::Coordinate& c, double v)
{
    auto rv = static_cast<float>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational) {
        rv = osc::Degrees{osc::Radians{rv}}.count();
    }

    return rv;
}

double opyn::convert_coord_display_value_to_storage_value(const OpenSim::Coordinate& c, float v)
{
    auto rv = static_cast<double>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational) {
        rv = osc::Radians{osc::Degrees{rv}}.count();
    }

    return rv;
}

osc::CStringView opyn::get_coord_display_value_units_string(const OpenSim::Coordinate& c)
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

std::vector<std::string> opyn::get_socket_names(const OpenSim::Component& c)
{
    return c.getSocketNames();
}

std::vector<const OpenSim::AbstractSocket*> opyn::get_all_sockets(const OpenSim::Component& c)
{
    std::vector<const OpenSim::AbstractSocket*> rv;

    for (const std::string& name : get_socket_names(c)) {
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

void opyn::write_component_topology_graph_as_dot_viz(
    const OpenSim::Component& root,
    std::ostream& out)
{
    std::set<GraphEdge> edges;

    // first, get all parent-to-child connections (easiest)
    for (const OpenSim::Component& child : root.getComponentList()) {
        const OpenSim::Component& parent = child.getOwner();

        edges.insert({
            .sourceAbsPath = get_absolute_path_string(parent),
            .destinationAbsPath = get_absolute_path_string(child),
            .name = "",
            .type = GraphEdgeType::ParentChild
        });
    }

    // helper: extract all socket edges leaving the given component
    auto extractSocketEdges = [&edges](const OpenSim::Component& c)
    {
        auto sourceAbsPath = get_absolute_path_string(c);
        for (const OpenSim::AbstractSocket* sock : get_all_sockets(c)) {
            if (const auto* connectee = dynamic_cast<const OpenSim::Component*>(&sock->getConnecteeAsObject())) {
                edges.insert({
                    .sourceAbsPath = sourceAbsPath,
                    .destinationAbsPath = get_absolute_path_string(*connectee),
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

void opyn::write_model_multibody_system_graph_as_dot_viz(
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

std::vector<OpenSim::AbstractSocket*> opyn::upd_all_sockets(OpenSim::Component& c)
{
    std::vector<OpenSim::AbstractSocket*> rv;

    for (const std::string& name : get_socket_names(c)) {
        rv.push_back(&c.updSocket(name));
    }

    return rv;
}

const OpenSim::Component* opyn::find_component(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return FindComponentGeneric(root, cp);
}

const OpenSim::Component* opyn::find_component(
    const OpenSim::Model& model,
    const std::string& absPath)
{
    return find_component(model, OpenSim::ComponentPath{absPath});
}

const OpenSim::Component* opyn::find_component(
    const OpenSim::Model& model,
    const osc::StringName& absPath)
{
    return find_component(model, std::string{absPath});
}

OpenSim::Component* opyn::find_component_mut(
    OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return FindComponentGeneric(root, cp);
}

bool opyn::contains_component(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& cp)
{
    return find_component(root, cp) != nullptr;
}

const OpenSim::AbstractSocket* opyn::find_socket(
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

OpenSim::AbstractSocket* opyn::find_socket_mut(
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

bool opyn::is_connected_to(
    const OpenSim::AbstractSocket& s,
    const OpenSim::Component& c)
{
    return &s.getConnecteeAsObject() == &c;
}

bool opyn::is_able_to_connect_to(
    const OpenSim::AbstractSocket& s,
    const OpenSim::Component& c)
{
    return s.canConnectTo(c);
}

void opyn::recursively_reassign_all_sockets(
    OpenSim::Component& root,
    const OpenSim::Component& from,
    const OpenSim::Component& to)
{
    for (OpenSim::Component& c : root.updComponentList()) {
        for (OpenSim::AbstractSocket* socket : upd_all_sockets(c)) {
            if (is_connected_to(*socket, from)) {
                TryConnectTo(*socket, to);
            }
        }
    }
}

std::ostream& opyn::operator<<(std::ostream& os, const ComponentConnectionView& view)
{
    return os << "ComponentConnectionView{source = " << view.source().getName() << ", target = " << view.target().getName() << ", socketName = " << view.socket_name() << '}';
}

osc::cpp23::generator<ComponentConnectionView> opyn::for_each_inbound_connection(
    const OpenSim::Component* root,
    const OpenSim::Component* c,
    std::function<bool(const OpenSim::Component&)> filter)
{
    for (const OpenSim::Component& subcomponent : root->getComponentList()) {
        if (not filter(subcomponent)) {
            continue;  // caller-provided filter stops emission
        }
        for (const auto& socketName : subcomponent.getSocketNames()) {
            if (const auto* socket = subcomponent.tryGetSocket(socketName)) {
                for (unsigned int i = 0; i < socket->getNumConnectees(); ++i) {
                    if (&socket->getConnecteeAsObject(static_cast<int>(i)) == c) {
                        co_yield ComponentConnectionView{
                            subcomponent,  // source
                            *c,            // target
                            socketName,    // connection name
                        };
                    }
                }
            }
        }
    }
}

OpenSim::AbstractProperty* opyn::find_property_mut(
    OpenSim::Component& c,
    const std::string& name)
{
    return c.hasProperty(name) ? &c.updPropertyByName(name) : nullptr;
}

const OpenSim::AbstractOutput* opyn::find_output(
    const OpenSim::Component& c,
    const std::string& outputName)
{
    const OpenSim::AbstractOutput* rv = nullptr;
    try {
        rv = &c.getOutput(outputName);
    }
    catch (const OpenSim::Exception&) {  // NOLINT(bugprone-empty-catch)
        // OpenSim, innit :(
    }
    return rv;
}

const OpenSim::AbstractOutput* opyn::find_output(
    const OpenSim::Component& root,
    const OpenSim::ComponentPath& path,
    const std::string& outputName)
{
    const OpenSim::Component* const c = find_component(root, path);
    return c ? find_output(*c, outputName) : nullptr;
}

bool opyn::has_input_file_name(const OpenSim::Model& m)
{
    const std::string& name = m.getInputFileName();
    return !name.empty() && name != "Unassigned";
}

std::optional<std::filesystem::path> opyn::try_find_input_file(const OpenSim::Model& m)
{
    if (not has_input_file_name(m)) {
        return std::nullopt;
    }

    std::filesystem::path p{m.getInputFileName()};
    if (not std::filesystem::exists(p)) {
        return std::nullopt;
    }

    return p;
}

std::string opyn::recommended_document_name(const OpenSim::Model& model)
{
    if (auto inputFile = try_find_input_file(model)) {
        return inputFile->filename().string();
    }
    else {
        return "untitled.osim";
    }
}

std::optional<std::filesystem::path> opyn::find_geometry_file_abs_path(
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

std::string opyn::get_mesh_file_name(const OpenSim::Mesh& mesh)
{
    std::filesystem::path p{mesh.get_mesh_file()};
    return p.filename().string();
}

bool opyn::should_show_in_ui(const OpenSim::Component& c)
{
    if (dynamic_cast<const OpenSim::PathWrapPoint*>(&c)) {
        return false;
    }

    if (dynamic_cast<const OpenSim::Station*>(&c) && owner_is<OpenSim::PathPoint>(c)) {
        return false;
    }

    return true;
}

bool opyn::try_delete_component_from_model(OpenSim::Model& m, OpenSim::Component& c)
{
    OpenSim::Component* const owner = upd_owner(m, c);

    if (!owner) {
        osc::log_error("cannot delete {}: it has no owner", c.getName());
        return false;
    }

    if (&c.getRoot() != &m) {
        osc::log_error("cannot delete {}: it is not owned by the provided model", c.getName());
        return false;
    }

    // check if anything connects to the component as a non-child (i.e. non-hierarchically)
    // via a socket to the component, which may break the other component (so halt deletion)
    if (auto connectees = GetAnyNonChildrenComponentsConnectedViaSocketTo(m, c); !connectees.empty())
    {
        std::stringstream ss;
        osc::CStringView delim;
        for (const OpenSim::Component* connectee : connectees) {
            ss << delim << connectee->getName();
            delim = ", ";
        }
        osc::log_error("cannot delete {}: the following components connect to it via sockets: {}", c.getName(), std::move(ss).str());
        return false;
    }

    // HACK: check if any path wraps connect to the component
    //
    // this is because the wrapping code isn't using sockets :< - this should be
    // fixed in OpenSim itself
    for (const OpenSim::PathWrap& pw : m.getComponentList<OpenSim::PathWrap>()) {
        if (pw.getWrapObject() == &c) {
            osc::log_error("cannot delete {}: it is used in a path wrap ({})", c.getName(), get_absolute_path_string(pw));
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
        rv = try_delete_item_from_set<OpenSim::ModelComponent, OpenSim::ModelComponent>(*componentSet, dynamic_cast<OpenSim::ModelComponent*>(&c));
    }
    else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(owner)) {
        rv = try_delete_item_from_set(*bs, dynamic_cast<OpenSim::Body*>(&c));
    }
    else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(owner)) {
        rv = try_delete_item_from_set(*wos, dynamic_cast<OpenSim::WrapObject*>(&c));
    }
    else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner)) {
        rv = try_delete_item_from_set(*cs, dynamic_cast<OpenSim::Controller*>(&c));
    }
    else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner)) {
        rv = try_delete_item_from_set(*conss, dynamic_cast<OpenSim::Constraint*>(&c));
    }
    else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner)) {
        rv = try_delete_item_from_set(*fs, dynamic_cast<OpenSim::Force*>(&c));
    }
    else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner)) {
        rv = try_delete_item_from_set(*ms, dynamic_cast<OpenSim::Marker*>(&c));
    }
    else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
        rv = try_delete_item_from_set(*cgs, dynamic_cast<OpenSim::ContactGeometry*>(&c));
    }
    else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner)) {
        rv = try_delete_item_from_set(*ps, dynamic_cast<OpenSim::Probe*>(&c));
    }
    else if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(owner)) {
        if (const auto* app = dynamic_cast<OpenSim::AbstractPathPoint*>(&c)) {
            rv = try_delete_item_from_set(gp->updPathPointSet(), app);
        }
        else if (const auto* pw = dynamic_cast<OpenSim::PathWrap*>(&c)) {
            rv = try_delete_item_from_set(gp->updWrapSet(), pw);
        }
    }
    else if (const auto* geom = dynamic_cast<OpenSim::Geometry*>(&c)) {
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
            auto copy = clone(prop);
            copy->clear();

            for (int i = 0; i < prop.size(); ++i) {
                if (OpenSim::Geometry& g = prop[i]; &g != geom) {
                    append(*copy, g);
                }
            }

            prop.assign(*copy);

            rv = true;
        }
    }
    else if (owner->removeComponent(&c)) {
        rv = true;
    }

    if (!rv) {
        osc::log_error("cannot delete {}: OpenSim Creator doesn't know how to delete a {} from its parent (maybe it can't?)", c.getName(), c.getConcreteClassName());
    }

    return rv;
}

void opyn::copy_common_joint_properties(const OpenSim::Joint& src, OpenSim::Joint& dest)
{
    dest.setName(src.getName());

    // copy owned frames
    dest.updProperty_frames().assign(src.getProperty_frames());

    // copy parent frame socket *path* (note: don't use connectSocket, pointers are evil in model manipulations)
    dest.updSocket("parent_frame").setConnecteePath(src.getSocket("parent_frame").getConnecteePath());

    // copy child socket *path* (note: don't use connectSocket, pointers are evil in model manipulations)
    dest.updSocket("child_frame").setConnecteePath(src.getSocket("child_frame").getConnecteePath());
}

bool opyn::deactivate_all_wrap_objects_in(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (size_t i = 0; i < size(wos); ++i) {
            OpenSim::WrapObject& wo = at(wos, i);
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
            rv = true;
        }
    }
    return rv;
}

bool opyn::activate_all_wrap_objects_in(OpenSim::Model& m)
{
    bool rv = false;
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (size_t i = 0; i < size(wos); ++i) {
            OpenSim::WrapObject& wo = at(wos, i);
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
            rv = true;
        }
    }
    return rv;
}

std::vector<const OpenSim::WrapObject*> opyn::get_all_wrap_objects_referenced_by(const OpenSim::GeometryPath& gp)
{
    const auto& wrapSet = gp.getWrapSet();

    std::vector<const OpenSim::WrapObject*> rv;
    rv.reserve(wrapSet.getSize());
    for (int i = 0; i < wrapSet.getSize(); ++i) {
        rv.push_back(wrapSet.get(i).getWrapObject());
    }
    return rv;

    // /bodyset/pelvis/pelvis_physicalbodyoffset/pelvis_geom
    // /bodyset/pelvis/pelvis_physicalbodyoffset/pelvis_geom
    // /bodyset/pelvis/pelvis_physicalbodyoffset/pelvis_geom
}

bool opyn::has_model_file_extension(const std::filesystem::path& path)
{
    // Some ".osim" files in the wild (e.g. on SimTK.org) have a capitalized extension
    // (e.g. "SomeOldModel.OSIM"). Although technically invalid on case-sensitive
    // filesystems/OSes, it should still be accepted (opensim-creator/#984).
    return osc::is_equal_case_insensitive(path.extension().string(), ".osim");
}

std::unique_ptr<OpenSim::Model> opyn::load_model(const std::filesystem::path& path)
{
    opyn::init();  // Ensure components are loaded etc.

    // HACK: OpenSim relies on global state changes (e.g. screwing around with
    // the process's current working directory) in order to load files, which
    // can cause problems when multiple threads try to load a model (opensim-creator/#1036).
    static std::mutex s_loading_mutex;
    std::lock_guard g{s_loading_mutex};
    return std::make_unique<OpenSim::Model>(path.string());
}

void opyn::initialize_model(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeModel");
    model.finalizeFromProperties();  // clears potentially-stale member components (required for `clearConnections`)
    model.clearConnections();        // clears any potentially stale pointers that can be retained by OpenSim::Socket<T> (see opensim-creator/#263)
    model.buildSystem();             // creates a new underlying physics system
}

void opyn::try_equilibrate_muscles_or_log_warning(OpenSim::Model& model, SimTK::State& state)
{
    try {
        model.equilibrateMuscles(state);
    }
    catch (const std::exception& ex) {
        osc::log_warn("Cannot equilibrate model's muscles: {}", ex.what());
    }
}

void opyn::finalize_connections(OpenSim::Model& model)
{
    OSC_PERF("osc::FinalizeConnections");
    model.finalizeConnections();
}

SimTK::State& opyn::initialize_state(OpenSim::Model& model)
{
    OSC_PERF("osc::InitializeState");
    SimTK::State& state = model.initializeState();  // creates+returns a new working state
    try_equilibrate_muscles_or_log_warning(model, state);
    model.realizeDynamics(state);
    return state;
}

void opyn::finalize_from_properties(OpenSim::Model& model)
{
    OSC_PERF("osc::FinalizeFromProperties");
    model.finalizeFromProperties();
}

std::optional<size_t> opyn::find_joint_in_parent_joint_set(const OpenSim::Joint& joint)
{
    const auto* parentJointSet = get_owner<OpenSim::JointSet>(joint);
    if (not parentJointSet) {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return std::nullopt;
    }

    return index_of(*parentJointSet, joint);
}

std::string opyn::get_display_name(const OpenSim::Geometry& g)
{
    if (const auto* mesh = dynamic_cast<const OpenSim::Mesh*>(&g); mesh) {
        return std::filesystem::path{mesh->getGeometryFilename()}.filename().string();
    }
    else {
        return g.getConcreteClassName();
    }
}

osc::CStringView opyn::get_motion_type_display_name(const OpenSim::Coordinate& c)
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

const OpenSim::Appearance* opyn::try_get_appearance(const OpenSim::Component& component)
{
    if (!component.hasProperty("Appearance")) {
        return nullptr;
    }

    const OpenSim::AbstractProperty& abstractProperty = component.getPropertyByName("Appearance");
    const auto* maybeAppearanceProperty = dynamic_cast<const OpenSim::Property<OpenSim::Appearance>*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->getValue() : nullptr;
}

OpenSim::Appearance* opyn::try_upd_appearance(OpenSim::Component& component)
{
    if (!component.hasProperty("Appearance")) {
        return nullptr;
    }

    OpenSim::AbstractProperty& abstractProperty = component.updPropertyByName("Appearance");
    auto* maybeAppearanceProperty = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&abstractProperty);

    return maybeAppearanceProperty ? &maybeAppearanceProperty->updValue() : nullptr;
}

bool opyn::try_set_appearance_property_is_visible_to(OpenSim::Component& c, bool v)
{
    if (OpenSim::Appearance* appearance = try_upd_appearance(c)) {
        appearance->set_visible(v);
        return true;
    }
    else {
        return false;
    }
}

osc::Color opyn::to_color(const OpenSim::Appearance& appearance)
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

osc::Color opyn::get_suggested_bone_color()
{
    const osc::Color usualDefault = {232.0f / 255.0f, 216.0f / 255.0f, 200.0f/255.0f, 1.0f};
    const float brightenAmount = 0.1f;
    return lerp(usualDefault, osc::Color::white(), brightenAmount);
}

bool opyn::is_showing_frames(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_frames();
}

bool opyn::toggle_showing_frames(OpenSim::Model& model)
{
    const bool newValue = !is_showing_frames(model);
    model.updDisplayHints().set_show_frames(newValue);
    return newValue;
}

bool opyn::is_showing_markers(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_markers();
}

bool opyn::toggle_showing_markers(OpenSim::Model& model)
{
    const bool newValue = !is_showing_markers(model);
    model.updDisplayHints().set_show_markers(newValue);
    return newValue;
}

bool opyn::is_showing_wrap_geometry(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_wrap_geometry();
}

bool opyn::toggle_showing_wrap_geometry(OpenSim::Model& model)
{
    const bool newValue = !is_showing_wrap_geometry(model);
    model.updDisplayHints().set_show_wrap_geometry(newValue);
    return newValue;
}

bool opyn::is_showing_contact_geometry(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_contact_geometry();
}

bool opyn::is_showing_forces(const OpenSim::Model& model)
{
    return model.getDisplayHints().get_show_forces();
}

bool opyn::toggle_showing_contact_geometry(OpenSim::Model& model)
{
    const bool newValue = !is_showing_contact_geometry(model);
    model.updDisplayHints().set_show_contact_geometry(newValue);
    return newValue;
}

bool opyn::toggle_showing_forces(OpenSim::Model& model)
{
    const bool newValue = !is_showing_forces(model);
    model.updDisplayHints().set_show_forces(newValue);
    return newValue;
}

void opyn::get_absolute_path_string(const OpenSim::Component& c, std::string& out)
{
    constexpr ptrdiff_t c_MaxEls = 16;

    ptrdiff_t nEls = 0;
    std::array<const OpenSim::Component*, c_MaxEls> els{};
    const OpenSim::Component* cur = &c;
    const OpenSim::Component* next = get_owner(c);

    if (!next) {
        // edge-case: caller provided a root
        out = '/';
        return;
    }

    while (cur && next && nEls < c_MaxEls) {
        els[nEls++] = cur;
        cur = next;
        next = get_owner(*cur);
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

std::string opyn::get_absolute_path_string(const OpenSim::Component& c)
{
    std::string rv;
    get_absolute_path_string(c, rv);
    return rv;
}

osc::StringName opyn::get_absolute_path_string_name(const OpenSim::Component& c)
{
    return osc::StringName{get_absolute_path_string(c)};
}

OpenSim::ComponentPath opyn::get_absolute_path(const OpenSim::Component& c)
{
    return OpenSim::ComponentPath{get_absolute_path_string(c)};
}

OpenSim::ComponentPath opyn::get_absolute_path_or_empty(const OpenSim::Component* c)
{
    if (c) {
        return get_absolute_path(*c);
    }
    else {
        return OpenSim::ComponentPath{};
    }
}

std::optional<LinesOfAction> opyn::get_effective_lines_of_action_in_ground(
    const OpenSim::Muscle& muscle,
    const SimTK::State& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = true;
    return TryGetLinesOfAction(muscle, state, config);
}

std::optional<LinesOfAction> opyn::get_anatomical_lines_of_action_in_ground(
    const OpenSim::Muscle& muscle,
    const SimTK::State& state)
{
    LinesOfActionConfig config;
    config.useEffectiveInsertion = false;
    return TryGetLinesOfAction(muscle, state, config);
}

namespace
{
    struct FirstContactHalfSpaceInHCF final {
        const OpenSim::ContactHalfSpace* ptr = nullptr;
        size_t index = 0;
    };

    // returns the first ContactHalfSpace found within the given HuntCrossleyForce's parameters, or
    // nullptr, if no ContactHalfSpace could be found
    std::optional<FirstContactHalfSpaceInHCF> FindFirstContactHalfSpaceInHCF(
        const OpenSim::Model& model,
        const OpenSim::HuntCrossleyForce& hcf)
    {
        // get contact parameters (i.e. where the contact geometry is stored)
        const OpenSim::HuntCrossleyForce::ContactParametersSet& paramSet = hcf.get_contact_parameters();
        if (empty(paramSet)) {
            return std::nullopt;  // edge-case: the force has no parameters
        }

        // linearly search for a ContactHalfSpace
        for (size_t i = 0; i < size(paramSet); ++i) {
            const OpenSim::HuntCrossleyForce::ContactParameters& param = at(paramSet, i);
            const OpenSim::Property<std::string>& geomProperty = param.getProperty_geometry();

            for (size_t j = 0; j < size(geomProperty); ++j) {
                const std::string& geomNameOrPath = at(geomProperty, j);
                if (const auto* foundViaAbsPath = find_component<OpenSim::ContactHalfSpace>(model, geomNameOrPath)) {
                    // found it as an abspath within the model
                    return FirstContactHalfSpaceInHCF{.ptr = foundViaAbsPath, .index = j};
                }
                else if (const auto* foundViaRelativePath = find_component<OpenSim::ContactHalfSpace>(model.getContactGeometrySet(), geomNameOrPath)) {
                    // found it as a relative path/name within the contactgeometryset
                    return FirstContactHalfSpaceInHCF{.ptr = foundViaRelativePath, .index = j};
                }
            }
        }
        return std::nullopt;
    }

    // helper: try to extract the current (state-dependent) force+torque from a HuntCrossleyForce
    struct ForceTorque final {
        osc::Vector3 force;
        osc::Vector3 torque;
    };
    std::optional<ForceTorque> CalcHCFForceOnContactHalfSpace(
        const OpenSim::HuntCrossleyForce& hcf,
        size_t firstContactHalfSpaceIndex,
        const SimTK::State& state)
    {
        const int offset = static_cast<int>(6*firstContactHalfSpaceIndex);

        const OpenSim::Array<double> forces = hcf.getRecordValues(state);
        if (forces.size() < offset + 6) {
            return std::nullopt;  // edge-case: OpenSim didn't report the expected forces
        }

        const osc::Vector3 force(-forces[offset+0], -forces[offset+1], -forces[offset+2]);
        if (osc::length2(force) < osc::epsilon_v<float>) {
            return std::nullopt;  // edge-case: no force is actually being exerted
        }

        const osc::Vector3 torque(-forces[offset+3], -forces[offset+4], -forces[offset+5]);

        return ForceTorque{force, torque};
    }

    // helper: convert an OpenSim::ContactHalfSpace, which is defined in a frame with an offset,
    //         etc. into a simpler "plane in ground-space" representation that's more useful
    //         for rendering
    osc::Plane ToPointNormalPlaneInGround(
        const OpenSim::ContactHalfSpace& halfSpace,
        const SimTK::State& state)
    {
        // go through the contact geometries that are attached to the force
        //
        // - if there's a plane, then the plane's location+normal are needed in order
        //   to figure out where the force is exerted
        const auto parent2ground = osc::to<osc::Transform>(halfSpace.getFrame().getTransformInGround(state));
        const auto halfspace2parent = osc::to<osc::Transform>(halfSpace.getTransform());

        return osc::Plane{
            .origin = parent2ground * halfspace2parent.translation,
            .normal = parent2ground.rotation * halfspace2parent.rotation * c_ContactHalfSpaceUpwardsNormal,
        };
    }

    // helper: returns the location of the center of pressure of a force+torque on a plane, or
    //         std::nullopt if the to-be-drawn force vector is too small
    std::optional<osc::Vector3> CalcCenterOfPressure(
        const osc::Plane& plane,
        const ForceTorque& forceTorqueInG)
    {
        const auto& [force, torque] = forceTorqueInG;
        if (osc::abs(osc::dot(plane.normal, force)) < osc::epsilon_v<float>) {
            return std::nullopt;  // the force vector is orthogonal to the plane's surface
        }

        // see also: SCONE/model_tools.cpp:GetPlaneCop
        auto normal_force_scalar = dot(force, plane.normal);
        auto pos0 = cross(plane.normal, torque) / normal_force_scalar;
        osc::Vector3 delta_pos = pos0 - plane.origin;
        double p1 = dot(delta_pos, plane.normal);
        double p2 = dot(force, plane.normal);
        auto pos = pos0 - (p1/p2) * force;
        return pos;
    }
}

std::optional<ForcePoint> opyn::try_get_contact_force_in_ground(
    const OpenSim::Model& model,
    const SimTK::State& state,
    const OpenSim::HuntCrossleyForce& hcf)
{
    // Find the first `OpenSim::ContactHalfSpace` in the HCF's contact set.
    auto const contactHalfSpace = FindFirstContactHalfSpaceInHCF(model, hcf);
    if (not contactHalfSpace) {
        return std::nullopt;  // couldn't find a ContactHalfSpace
    }

    // Calculate the equivalent point-normal `Plane` for the `OpenSim::ContactHalfSpace`.
    const osc::Plane contactPlaneInG = ToPointNormalPlaneInGround(*contactHalfSpace->ptr, state);

    // Extract the force+torque that the HCF is applying to the `OpenSim::ContactHalfSpace`.
    const auto forceTorqueAppliedToPlane = CalcHCFForceOnContactHalfSpace(hcf, contactHalfSpace->index, state);
    if (not forceTorqueAppliedToPlane) {
        return std::nullopt;  // couldn't extract force+torque from the HCF
    }

    // Use the force+torque applied to the `Plane`'s origin to calculate the center of pressure.
    const auto contactPlaneCOP = CalcCenterOfPressure(contactPlaneInG, *forceTorqueAppliedToPlane);
    if (not contactPlaneCOP) {
        return std::nullopt;  // the resulting force is too small
    }

    return ForcePoint{forceTorqueAppliedToPlane->force, *contactPlaneCOP};
}

const OpenSim::PhysicalFrame& opyn::get_frame_using_external_force_lookup_heuristic(
    const OpenSim::Model& model,
    const std::string& bodyNameOrPath)
{
    // this tries to match the implementation that's hidden inside
    // of `ExternalForce.cpp` from OpenSim

    if (const auto* direct = find_component<OpenSim::PhysicalFrame>(model, bodyNameOrPath)) {
        return *direct;
    }
    else if (const auto* shimmed = find_component<OpenSim::PhysicalFrame>(model, "./bodyset/" + bodyNameOrPath)) {
        return *shimmed;
    }
    else {
        return model.getGround();
    }
}

bool opyn::can_extract_point_info_from(const OpenSim::Component& c, const SimTK::State& st)
{
    return try_extract_point_info(c, st) != std::nullopt;
}

std::optional<PointInfo> opyn::try_extract_point_info(
    const OpenSim::Component& c,
    const SimTK::State& st)
{
    if (dynamic_cast<const OpenSim::PathWrapPoint*>(&c)) {
        // HACK: path wrap points don't update the cache correctly?
        return std::nullopt;
    }
    if (const auto* station = dynamic_cast<const OpenSim::Station*>(&c)) {
        // HACK: OpenSim redundantly stores path point information in a child called 'station'.
        // These must be filtered because, otherwise, the user will just see a bunch of
        // 'station' entries below each path point
        if (station->getName() == "station" && owner_is<OpenSim::PathPoint>(*station)) {
            return std::nullopt;
        }

        return PointInfo{
            osc::to<osc::Vector3>(station->get_location()),
            get_absolute_path(station->getParentFrame()),
        };
    }
    if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&c)) {
        return PointInfo{
            osc::to<osc::Vector3>(pp->getLocation(st)),
            get_absolute_path(pp->getParentFrame()),
        };
    }
    if (const auto* point = dynamic_cast<const OpenSim::Point*>(&c)) {
        return PointInfo{
            osc::to<osc::Vector3>(point->getLocationInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }
    if (const auto* frame = dynamic_cast<const OpenSim::Frame*>(&c)) {
        return PointInfo{
            osc::to<osc::Vector3>(frame->getPositionInGround(st)),
            OpenSim::ComponentPath{"/ground"},
        };
    }

    return std::nullopt;
}

OpenSim::Component& opyn::add_component_to_appropriate_set(OpenSim::Model& m, std::unique_ptr<OpenSim::Component> c)
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

OpenSim::ModelComponent& opyn::add_model_component(OpenSim::Model& model, std::unique_ptr<OpenSim::ModelComponent>&& p)
{
    OpenSim::ModelComponent& rv = *p;
    model.addModelComponent(std::move(p).release());
    return rv;
}

OpenSim::Component& opyn::add_component(OpenSim::Component& c, std::unique_ptr<OpenSim::Component>&& p)
{
    OpenSim::Component& rv = *p;
    c.addComponent(std::move(p).release());
    return rv;
}

OpenSim::Body& opyn::add_body(OpenSim::Model& model, std::unique_ptr<OpenSim::Body> p)
{
    OpenSim::Body& rv = *p;
    model.addBody(p.release());
    return rv;
}

OpenSim::Joint& opyn::add_joint(OpenSim::Model& model, std::unique_ptr<OpenSim::Joint> j)
{
    OpenSim::Joint& rv = *j;
    model.addJoint(j.release());
    return rv;
}

OpenSim::Constraint& opyn::add_constraint(OpenSim::Model& model, std::unique_ptr<OpenSim::Constraint> constraint)
{
    OpenSim::Constraint& rv = *constraint;
    model.addConstraint(constraint.release());
    return rv;
}

OpenSim::Marker& opyn::add_marker(OpenSim::Model& model, std::unique_ptr<OpenSim::Marker> marker)
{
    OpenSim::Marker& rv = *marker;
    model.addMarker(marker.release());
    return rv;
}

OpenSim::PhysicalOffsetFrame& opyn::add_frame(OpenSim::Joint& joint, std::unique_ptr<OpenSim::PhysicalOffsetFrame> frame)
{
    OpenSim::PhysicalOffsetFrame& rv = *frame;
    joint.addFrame(frame.release());
    return rv;
}

OpenSim::WrapObject& opyn::add_wrap_object(OpenSim::PhysicalFrame& physFrame, std::unique_ptr<OpenSim::WrapObject> wrapObj)
{
    OpenSim::WrapObject& rv = *wrapObj;
    physFrame.addWrapObject(wrapObj.release());
    return rv;
}

OpenSim::Geometry& opyn::attach_geometry(OpenSim::Frame& frame, std::unique_ptr<OpenSim::Geometry> p)
{
    OpenSim::Geometry& rv = *p;
    frame.attachGeometry(p.release());
    return rv;
}

void opyn::overwrite_geometry(
    OpenSim::Model& model,
    OpenSim::Geometry& oldGeometry,
    std::unique_ptr<OpenSim::Geometry> newGeometry)
{
    newGeometry->set_scale_factors(oldGeometry.get_scale_factors());
    newGeometry->set_Appearance(oldGeometry.get_Appearance());
    newGeometry->updSocket("frame").setConnecteePath(oldGeometry.getSocket("frame").getConnecteePath());
    newGeometry->setName(oldGeometry.getName());
    OpenSim::Component* owner = upd_owner(model, oldGeometry);
    OSC_ASSERT_ALWAYS(owner && "the mesh being replaced has no owner? cannot overwrite a root component");
    OSC_ASSERT_ALWAYS(try_delete_component_from_model(model, oldGeometry) && "cannot delete old mesh from model during warping");
    initialize_model(model);
    initialize_state(model);
    // HACK: prefer `<attachedGeometry>` block when overwriting meshes defined
    // in frames, because we don't have a way to delete things from the generic
    // component list (yet) opensim-creator/#1003.
    if (auto* fr = dynamic_cast<OpenSim::Frame*>(owner)) {
        fr->attachGeometry(newGeometry.release());
    }
    else {
        owner->addComponent(newGeometry.release());
    }

    finalize_connections(model);
}

const OpenSim::PhysicalFrame* opyn::try_get_parent_to_ground_frame(const OpenSim::Component& component)
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

std::optional<SimTK::Transform> opyn::try_get_parent_to_ground_transform(
    const OpenSim::Component& component,
    const SimTK::State& state)
{
    if (const OpenSim::PhysicalFrame* frame = try_get_parent_to_ground_frame(component)) {
        return frame->getTransformInGround(state);
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::string> opyn::try_get_positional_property_name(
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

std::optional<std::string> opyn::try_get_orientational_property_name(
    const OpenSim::Component& component)
{
    if (const auto* pof = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&component)) {
        return pof->getProperty_orientation().getName();
    }
    else {
        return std::nullopt;
    }
}

const OpenSim::Frame* opyn::try_get_parent_frame(const OpenSim::Frame& frame)
{
    if (auto offset = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&frame)) {
        return &offset->getParentFrame();
    }
    return nullptr;
}

std::optional<ComponentSpatialRepresentation> opyn::try_get_spatial_representation(
    const OpenSim::Component& component,
    const SimTK::State& state)
{
    if (auto xform = try_get_parent_to_ground_transform(component, state)) {
        if (auto posProp = try_get_positional_property_name(component)) {
            return ComponentSpatialRepresentation{
                *xform,
                std::move(posProp).value(),
                try_get_orientational_property_name(component)
            };
        }
    }
    return std::nullopt;
}

bool opyn::is_valid_open_sim_component_name_character(char c)
{
    return
        std::isalpha(static_cast<unsigned char>(c)) != 0 ||
        ('0' <= c && c <= '9') ||
        (c == '-' || c == '_');
}

std::string opyn::sanitize_to_open_sim_component_name(std::string_view sv)
{
    std::string rv;
    for (auto c : sv) {
        if (is_valid_open_sim_component_name_character(c)) {
            rv += c;
        }
    }
    return rv;
}

std::unique_ptr<OpenSim::Storage> opyn::load_storage(
    const OpenSim::Model& model,
    const std::filesystem::path& path,
    const StorageLoadingParameters& params)
{
    auto rv = std::make_unique<OpenSim::Storage>(path.string());

    if (params.convert_rotational_values_to_radians and rv->isInDegrees()) {
        model.getSimbodyEngine().convertDegreesToRadians(*rv);
    }

    if (params.resample_to_frequency) {
        rv->resampleLinear(*params.resample_to_frequency);
    }

    return rv;
}

std::unordered_map<int, int> opyn::create_storage_index_to_model_statevar_mapping_with_warnings(
    const OpenSim::Model& model,
    const OpenSim::Storage& storage)
{
    auto mapping = create_storage_index_to_model_statevar_mapping(model, storage);
    if (not mapping.state_variables_missing_in_storage.empty()) {
        std::stringstream ss;
        ss << "the provided STO file is missing the following columns:\n";
        std::string_view delimiter;
        for (const std::string& el : mapping.state_variables_missing_in_storage) {
            ss << delimiter << el;
            delimiter = ", ";
        }
        osc::log_warn(std::move(ss).str());
        osc::log_warn("The STO file was loaded successfully, but beware: the missing state variables have been defaulted in order for this to work");
        osc::log_warn("Therefore, do not treat the motion you are seeing as a 'true' representation of something: some state data was 'made up' to make the motion viewable");
    }
    return std::move(mapping.storage_index_to_model_state_var_index);
}

StorageIndexToModelStateVarMappingResult opyn::create_storage_index_to_model_statevar_mapping(
    const OpenSim::Model& model,
    const OpenSim::Storage& storage)
{
    // ensure the `OpenSim::Storage` holds a time sequence.
    if (not osc::is_equal_case_insensitive(storage.getColumnLabels()[0], "time")) {
        throw std::runtime_error{"the provided motion data does not contain a 'time' column as its first column: it cannot be processed"};
    }

    // get+validate column headers from the `OpenSim::Storage`.
    const OpenSim::Array<std::string>& storageColumnsIncludingTime = storage.getColumnLabels();
    if (not is_all_elements_unique(storageColumnsIncludingTime)) {
        throw std::runtime_error{"the provided motion data contains multiple columns with the same name. This creates ambiguities that OpenSim Creator can't handle"};
    }

    // get the state variable labels from the `OpenSim::Model`
    OpenSim::Array<std::string> modelStateVars = model.getStateVariableNames();

    StorageIndexToModelStateVarMappingResult rv;
    rv.storage_index_to_model_state_var_index.reserve(modelStateVars.size());

    // compute storage-to-model index mapping
    //
    // care: The storage's column labels do not match the model's state variable names
    //       1:1. STO files have changed over time. OpenSim pre-4.0 used different naming
    //       conventions for the column labels, so you *need* to map the storage column
    //       strings carefully onto the model state variables.
    for (int modelIndex = 0; modelIndex < modelStateVars.size(); ++modelIndex) {
        const std::string& modelStateVarname = modelStateVars[modelIndex];
        const int storageIndex = OpenSim::TableUtilities::findStateLabelIndex(storageColumnsIncludingTime, modelStateVarname);
        const int valueIndex = storageIndex - 1;  // the column labels include 'time', which isn't in the data elements

        if (valueIndex >= 0) {
            rv.storage_index_to_model_state_var_index[valueIndex] = modelIndex;
        }
        else {
            rv.state_variables_missing_in_storage.push_back(modelStateVarname);
        }
    }

    return rv;
}

void opyn::update_state_variables_from_storage_row(
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
            throw std::runtime_error{"an index in the storage lookup was invalid: this is probably a developer error that needs to be investigated (report it)"};
        }
    }

    // update state with new state variables and re-assemble, re-realize, etc.
    state.setTime(sv->getTime());
    for (auto& coordinate : model.getComponentList<OpenSim::Coordinate>()) {
        coordinate.setLocked(state, false);
    }
    model.setStateVariableValues(state, stateValsBuf);
}

void opyn::update_state_from_storage_time(
    OpenSim::Model& model,
    SimTK::State& state,
    const std::unordered_map<int, int>& columnIndexToModelStateVarIndex,
    const OpenSim::Storage& storage,
    double time)
{
    update_state_variables_from_storage_row(model, state, columnIndexToModelStateVarIndex, storage, storage.findIndex(time));
}

std::string opyn::write_object_xml_to_string(const OpenSim::Object& obj)
{
    SimTK::Xml::Document d;
    SimTK::Xml::Element el = d.getRootElement();
    obj.updateXMLNode(el);
    if (el.element_begin() != el.element_end()) {
        SimTK::String str;
        el.element_begin()->writeToString(str);
        return str;
    }
    else {
        return {};
    }
}

void opyn::scale_model_mass_preserve_mass_distribution(
    OpenSim::Model& model,
    const SimTK::State& state,
    double newMass)
{
    // This is a simplified version of part of `OpenSim::Model::scale`

    const double modelMass = model.getTotalMass(state);
    OSC_ASSERT_ALWAYS(modelMass != 0.0 && "Cannot scale the mass of a model that has a mass of zero");

    const double factor = newMass / modelMass;
    for (OpenSim::Body& body : model.updComponentList<OpenSim::Body>()) {
        body.scaleMass(factor);
    }
}

void opyn::bake_station_defined_frames(OpenSim::Model& model)
{
    // Mutate the model by adding equivalent `PhysicalOffsetFrame`s to the
    // model, reattaching stuff to it, and then deleting the `StationDefinedFrame`.

    model.finalizeConnections();
    std::vector<OpenSim::StationDefinedFrame*> sdfsToDelete;
    std::vector<OpenSim::PhysicalOffsetFrame*> pofsToRename;
    for (auto& sdf : model.updComponentList<OpenSim::StationDefinedFrame>()) {

        // Create a new `PhysicalOffsetFrame`
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();

        // Copy/calculate properties for the `PhysicalOffsetFrame`
        pof->setName(sdf.getName() + "_tmp");
        const SimTK::Transform xform = sdf.findTransformInBaseFrame();
        pof->set_translation(xform.p());
        pof->set_orientation(xform.R().convertRotationToBodyFixedXYZ());
        pof->updProperty_attached_geometry().assign(sdf.getProperty_attached_geometry());
        pof->updProperty_WrapObjectSet().assign(sdf.getProperty_WrapObjectSet());
        pof->updSocket("parent").setConnecteePath(sdf.findBaseFrame().getAbsolutePathString());
        pof->updPropertyByName("components").assign(sdf.getPropertyByName("components"));

        // Add it into the model
        auto& pofPtr = *pof;
        model.updComponent(sdf.getAbsolutePath().getParentPath()).addComponent(pof.release());
        pofPtr.finalizeConnections(model);
        // Reassign anything pointing to the SDF to instead point to the POF
        recursively_reassign_all_sockets(model,sdf, pofPtr);
        sdfsToDelete.push_back(&sdf);
        pofsToRename.push_back(&pofPtr);
    }
    for (size_t i = 0; i < sdfsToDelete.size(); ++i) {
        std::string name = sdfsToDelete[i]->getName();
        try_delete_component_from_model(model, *sdfsToDelete[i]);
        pofsToRename[i]->setName(name);
    }
    finalize_connections(model);
    initialize_model(model);
    initialize_state(model);
}
