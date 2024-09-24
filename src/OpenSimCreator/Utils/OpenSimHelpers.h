#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/PointDirection.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/StringName.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class AbstractPathPoint; }
namespace OpenSim { class AbstractProperty; }
namespace OpenSim { class AbstractSocket; }
namespace OpenSim { class Appearance; }
namespace OpenSim { template<typename> class Array; }
namespace OpenSim { template<typename> class ArrayPtrs; }
namespace OpenSim { class Body; }
namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class ExternalForce; }
namespace OpenSim { class Frame; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class GeometryPath; }
namespace OpenSim { class HuntCrossleyForce; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Marker; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelComponent; }
namespace OpenSim { class Muscle; }
namespace OpenSim { class Object; }
namespace OpenSim { class PointForceDirection; }
namespace OpenSim { template<typename> class Property; }
namespace OpenSim { template<typename> class ObjectProperty; }
namespace OpenSim { class PhysicalFrame; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { template<typename, typename> class Set; }
namespace OpenSim { template<typename> class SimpleProperty; }
namespace OpenSim { class Storage; }
namespace OpenSim { class WrapObject; }
namespace SimTK { class State; }
namespace SimTK { class SimbodyMatterSubsystem; }

// OpenSimHelpers: a collection of various helper functions that are used by `osc`
namespace osc
{
    // Is satisfied if `T` has a `.clone()` member method that returns a raw `T*` pointer.
    template<typename T>
    concept ClonesToRawPointer = requires(const T& v) {
        { v.clone() } -> std::same_as<T*>;
    };

    // Returns the number of elements in `s` (see: `std::size`).
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    size_t size(const OpenSim::Set<T, C>& s)
    {
        return static_cast<size_t>(s.getSize());
    }

    // Returns the number of elements in `s` as a signed integer (see: `std::ssize`).
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    ptrdiff_t ssize(const OpenSim::Set<T, C>& s)
    {
        return static_cast<ptrdiff_t>(s.getSize());
    }

    // Returns the number of elements in `ary` (see: `std::size`).
    template<typename T>
    size_t size(const OpenSim::ArrayPtrs<T>& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    // Returns the number of elements in `ary` (see: `std::size`).
    template<typename T>
    size_t size(const OpenSim::Array<T>& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    // Returns the number of elements in `p` (see `std::size`).
    template<typename T>
    size_t size(const OpenSim::Property<T>& p)
    {
        return static_cast<size_t>(p.size());
    }

    // Returns an iterator to the beginning of `ary` (see: `std::begin`, `std::ranges::begin`).
    template<typename T>
    const T* begin(const OpenSim::Array<T>& ary)
    {
        return std::addressof(ary[0]);
    }

    // Returns an iterator to the beginning of `ary` (see: `std::begin`, `std::ranges::begin`).
    template<typename T>
    T* begin(OpenSim::Array<T>& ary)
    {
        return std::addressof(ary[0]);
    }

    // Returns an iterator to the end (i.e. the element after the last element) of `ary` (see: `std::end`, `std::ranges::end`)
    template<typename T>
    const T* end(const OpenSim::Array<T>& ary)
    {
        return std::addressof(ary[ary.getSize()]);
    }

    // Returns an iterator to the end (i.e. the element after the last element) of `ary` (see: `std::end`, `std::ranges::end`)
    template<typename T>
    T* end(OpenSim::Array<T>& ary)
    {
        return std::addressof(ary[ary.getSize()]);
    }

    // Returns whether `s` is empty (see: `std::empty`)
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    [[nodiscard]] bool empty(const OpenSim::Set<T, C>& s)
    {
        return size(s) <= 0;
    }

    // Returns a reference to the element of `ary` at location `pos`, with bounds checking.
    template<typename T>
    T& At(OpenSim::ArrayPtrs<T>& ary, size_t pos)
    {
        if (pos >= size(ary)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }

        if (T* el = ary.get(static_cast<int>(pos))) {
            return *el;
        }
        else {
            throw std::runtime_error{"attempted to access null element of ArrayPtrs"};
        }
    }

    // Returns a reference to the element of `ary` at location `pos`, with bounds checking.
    template<typename T>
    const T& At(const OpenSim::Array<T>& ary, size_t pos)
    {
        if (pos >= size(ary)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }
        return ary.get(static_cast<int>(pos));
    }

    // Returns a reference to the element of `s` at location `pos`, with bounds checking.
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    const T& At(const OpenSim::Set<T, C>& s, size_t pos)
    {
        if (pos >= size(s)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        return s.get(static_cast<int>(pos));
    }

    // Returns a reference to the element of `s` at location `pos`, with bounds checking.
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    T& At(OpenSim::Set<T, C>& s, size_t pos)
    {
        if (pos >= size(s)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        s.get(static_cast<int>(pos));  // force an OpenSim-based null check
        return s[static_cast<int>(pos)];
    }

    // Returns a reference to the element of `p` at location `pos`, with bounds checking.
    template<typename T>
    const T& At(const OpenSim::Property<T>& p, size_t pos)
    {
        return p[static_cast<int>(pos)];  // the implementation is already bounds-checked
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    bool EraseAt(OpenSim::Set<T, C>& s, size_t i)
    {
        return s.remove(static_cast<int>(i));
    }

    // Returns whether all elements in `v` are not equal to any other element in `v`.
    template<typename T>
    requires (std::strict_weak_order<std::ranges::less, T, T> and std::equality_comparable<T>)
    bool IsAllElementsUnique(const OpenSim::Array<T>& v)
    {
        std::vector<std::reference_wrapper<const T>> buffer;
        buffer.reserve(v.size());
        std::ranges::copy(begin(v), end(v), std::back_inserter(buffer));
        std::ranges::sort(buffer, std::less<T>{});
        return std::ranges::adjacent_find(buffer, std::ranges::equal_to{}, [](const auto& wrapper) { return wrapper.get(); }) == buffer.end();
    }

    // returns true if the first argument has a lexographically lower class name
    bool IsConcreteClassNameLexographicallyLowerThan(
        const OpenSim::Component&,
        const OpenSim::Component&
    );

    // returns true if the first argument points to a component that has a lexographically lower class
    // name than the component pointed to by second argument
    //
    // (it's a helper method that's handy for use with pointers, unique_ptr, shared_ptr, etc.)
    template<DereferencesTo<const OpenSim::Component&> ComponentPtrLike>
    bool IsConcreteClassNameLexographicallyLowerThan(
        const ComponentPtrLike& a,
        const ComponentPtrLike& b)
    {
        return IsConcreteClassNameLexographicallyLowerThan(*a, *b);
    }

    bool IsNameLexographicallyLowerThan(
        const OpenSim::Component&,
        const OpenSim::Component&
    );

    template<DereferencesTo<const OpenSim::Component&> Ptr>
    bool IsNameLexographicallyLowerThan(
        const Ptr& a,
        const Ptr& b)
    {
        return IsNameLexographicallyLowerThan(*a, *b);
    }

    template<DereferencesTo<const OpenSim::Component&> Ptr>
    bool IsNameLexographicallyGreaterThan(
        const Ptr& a,
        const Ptr& b)
    {
        return !IsNameLexographicallyLowerThan<Ptr>(a, b);
    }

    // returns a mutable pointer to the owner (if it exists)
    OpenSim::Component* UpdOwner(OpenSim::Component& root, const OpenSim::Component&);
    OpenSim::Component& UpdOwnerOrThrow(OpenSim::Component& root, const OpenSim::Component&);

    template<std::derived_from<OpenSim::Component> T>
    T& UpdOwnerOrThrow(OpenSim::Component& root, const OpenSim::Component& c)
    {
        return dynamic_cast<T&>(UpdOwnerOrThrow(root, c));
    }

    template<std::derived_from<OpenSim::Component> T>
    T* UpdOwner(OpenSim::Component& root, const OpenSim::Component& c)
    {
        return dynamic_cast<T*>(UpdOwner(root, c));
    }

    // returns a pointer to the owner (if it exists)
    const OpenSim::Component& GetOwnerOrThrow(const OpenSim::AbstractOutput&);
    const OpenSim::Component& GetOwnerOrThrow(const OpenSim::Component&);
    const OpenSim::Component& GetOwnerOr(const OpenSim::Component&, const OpenSim::Component& fallback);
    const OpenSim::Component* GetOwner(const OpenSim::Component&);

    template<std::derived_from<OpenSim::Component> T>
    const T* GetOwner(const OpenSim::Component& c)
    {
        return dynamic_cast<const T*>(GetOwner(c));
    }

    template<std::derived_from<OpenSim::Component> T>
    bool OwnerIs(const OpenSim::Component& c)
    {
        return GetOwner<T>(c) != nullptr;
    }

    std::optional<std::string> TryGetOwnerName(const OpenSim::Component&);

    // returns the distance between the given `Component` and the component that is at the root of the component tree
    size_t DistanceFromRoot(const OpenSim::Component&);

    // returns a reference to a global instance of a path that points to the root of a model (i.e. "/")
    OpenSim::ComponentPath GetRootComponentPath();

    // returns `true` if the given `ComponentPath` is an empty path
    bool IsEmpty(const OpenSim::ComponentPath&);

    // clears the given component path
    void Clear(OpenSim::ComponentPath&);

    // returns all components between the root (element 0) and the given component (element n-1) inclusive
    std::vector<const OpenSim::Component*> GetPathElements(const OpenSim::Component&);

    // calls the given function with each subcomponent of the given component
    void ForEachComponent(const OpenSim::Component&, const std::function<void(const OpenSim::Component&)>&);

    // calls the given function with the provided component and each of its subcomponents
    void ForEachComponentInclusive(const OpenSim::Component&, const std::function<void(const OpenSim::Component&)>&);

    // returns the number of children (recursive) of type T under the given component
    template<std::derived_from<OpenSim::Component> T>
    size_t GetNumChildren(const OpenSim::Component& c)
    {
        size_t i = 0;
        ForEachComponent(c, [&i](const OpenSim::Component& c)
        {
            if (dynamic_cast<const T*>(&c)) {
                ++i;
            }
        });
        return i;
    }

    // returns the number of direct children that the component owns
    size_t GetNumChildren(const OpenSim::Component&);

    // returns `true` if `c == parent` or `c` is a descendent of `parent`
    bool IsInclusiveChildOf(
        const OpenSim::Component* parent,
        const OpenSim::Component* c
    );

    // returns the first parent in `parents` that appears to be an inclusive parent of `c`
    //
    // returns `nullptr` if no element in `parents` is an inclusive parent of `c`
    const OpenSim::Component* IsInclusiveChildOf(
        std::span<const OpenSim::Component*> parents,
        const OpenSim::Component* c
    );

    // returns the first ancestor of `c` for which the given predicate returns `true`
    const OpenSim::Component* FindFirstAncestorInclusive(
        const OpenSim::Component*,
        bool(*pred)(const OpenSim::Component*)
    );

    // returns the first ancestor of `c` that has type `T`
    template<std::derived_from<OpenSim::Component> T>
    const T* FindAncestorWithType(const OpenSim::Component* c)
    {
        const OpenSim::Component* rv = FindFirstAncestorInclusive(c, [](const OpenSim::Component* el)
        {
            return dynamic_cast<const T*>(el) != nullptr;
        });

        return dynamic_cast<const T*>(rv);
    }

    // returns `true` if `c` is a child of a component that derives from `T`
    template<std::derived_from<OpenSim::Component> T>
    bool IsChildOfA(const OpenSim::Component& c)
    {
        return FindAncestorWithType<T>(&c) != nullptr;
    }

    // returns the first descendent (including `component`) that satisfies `predicate(descendent);`
    const OpenSim::Component* FindFirstDescendentInclusive(
        const OpenSim::Component& component,
        bool(*predicate)(const OpenSim::Component&)
    );

    // returns the first descendent of `component` that satisfies `predicate(descendent)`
    const OpenSim::Component* FindFirstDescendent(
        const OpenSim::Component& component,
        bool(*predicate)(const OpenSim::Component&)
    );

    // returns the first direct descendent of `component` that has type `T`, or
    // `nullptr` if no such descendent exists
    template<std::derived_from<OpenSim::Component> T>
    const T* FindFirstDescendentOfType(const OpenSim::Component& c)
    {
        const OpenSim::Component* rv = FindFirstDescendent(c, [](const OpenSim::Component& el)
        {
            return dynamic_cast<const T*>(&el) != nullptr;
        });
        return dynamic_cast<const T*>(rv);
    }

    // returns a vector containing pointers to all user-editable coordinates in the model
    std::vector<const OpenSim::Coordinate*> GetCoordinatesInModel(const OpenSim::Model&);

    // fills the given vector with all user-editable coordinates in the model
    void GetCoordinatesInModel(
        const OpenSim::Model&,
        std::vector<const OpenSim::Coordinate*>&
    );

    // returns a vector containing mutable pointers to all default-locked coordinates in the model
    std::vector<OpenSim::Coordinate*> UpdDefaultLockedCoordinatesInModel(OpenSim::Model&);

    // returns the user-facing display value (i.e. degrees) for a coordinate
    float ConvertCoordValueToDisplayValue(const OpenSim::Coordinate&, double v);

    // returns the storage-facing value (i.e. radians) for a coordinate
    double ConvertCoordDisplayValueToStorageValue(const OpenSim::Coordinate&, float v);

    // returns a user-facing string that describes a coordinate's units
    CStringView GetCoordDisplayValueUnitsString(const OpenSim::Coordinate&);

    // returns the names of a component's sockets
    std::vector<std::string> GetSocketNames(const OpenSim::Component&);

    // returns all sockets that are directly attached to the given component
    std::vector<const OpenSim::AbstractSocket*> GetAllSockets(const OpenSim::Component&);

    // returns all (mutable) sockets that are directly attached to the given component
    std::vector<OpenSim::AbstractSocket*> UpdAllSockets(OpenSim::Component&);

    // writes the given component's (recursive) topology graph to the output stream as a
    // dotviz `digraph`
    void WriteComponentTopologyGraphAsDotViz(
        const OpenSim::Component&,
        std::ostream&
    );

    // writes the given model's multibody system (i.e. kinematic chain) to the output stream
    // as a dotviz `digraph`
    void WriteModelMultibodySystemGraphAsDotViz(
        const OpenSim::Model&,
        std::ostream&
    );

    // returns a pointer if the given path resolves a component relative to root
    const OpenSim::Component* FindComponent(const OpenSim::Component& root, const OpenSim::ComponentPath&);
    const OpenSim::Component* FindComponent(const OpenSim::Model&, const std::string& absPath);
    const OpenSim::Component* FindComponent(const OpenSim::Model&, const StringName& absPath);

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    const T* FindComponent(const OpenSim::Component& root, const OpenSim::ComponentPath& cp)
    {
        return dynamic_cast<const T*>(FindComponent(root, cp));
    }

    template<std::derived_from<OpenSim::Component> T>
    const T* FindComponent(const OpenSim::Model& root, const std::string& cp)
    {
        return dynamic_cast<const T*>(FindComponent(root, cp));
    }

    template<std::derived_from<OpenSim::Component> T>
    const T* FindComponent(const OpenSim::Model& root, const StringName& cp)
    {
        return dynamic_cast<const T*>(FindComponent(root, cp));
    }

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* FindComponentMut(
        OpenSim::Component& root,
        const OpenSim::ComponentPath&
    );

    // returns non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    T* FindComponentMut(
        OpenSim::Component& root,
        const OpenSim::ComponentPath& cp)
    {
        return dynamic_cast<T*>(FindComponentMut(root, cp));
    }

    // returns true if the path resolves to a component within `root`
    bool ContainsComponent(
        const OpenSim::Component& root,
        const OpenSim::ComponentPath&
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    const OpenSim::AbstractSocket* FindSocket(
        const OpenSim::Component&,
        const std::string& socketName
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    OpenSim::AbstractSocket* FindSocketMut(
        OpenSim::Component&,
        const std::string& socketName
    );

    // returns `true` if the socket is connected to the component
    bool IsConnectedTo(
        const OpenSim::AbstractSocket&,
        const OpenSim::Component&
    );

    // returns true if the socket is able to connect to the component
    bool IsAbleToConnectTo(
        const OpenSim::AbstractSocket&,
        const OpenSim::Component&
    );

    // recursively traverses all components within `root` and reassigns any sockets
    // pointing to `from` to instead point to `to`
    //
    // note: must be called on a component that already has finalized connections
    void RecursivelyReassignAllSockets(
        OpenSim::Component& root,
        const OpenSim::Component& from,
        const OpenSim::Component& to
    );

    // returns a pointer to the property if the component has a property with the given name
    OpenSim::AbstractProperty* FindPropertyMut(
        OpenSim::Component&,
        const std::string&
    );

    // returns a pointer to the property if the component has a simple property with the given name and type
    template<typename T>
    OpenSim::SimpleProperty<T>* FindSimplePropertyMut(
        OpenSim::Component& c,
        const std::string& name)
    {
        return dynamic_cast<OpenSim::SimpleProperty<T>*>(FindPropertyMut(c, name));
    }

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to the given component
    const OpenSim::AbstractOutput* FindOutput(
        const OpenSim::Component&,
        const std::string& outputName
    );

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to a component located at the given path relative to the root
    const OpenSim::AbstractOutput* FindOutput(
        const OpenSim::Component& root,
        const OpenSim::ComponentPath&,
        const std::string& outputName
    );

    // returns true if the given model has an input file name (not empty, or "Unassigned")
    bool HasInputFileName(const OpenSim::Model&);

    // returns a non-empty path if the given model has an input file name that exists on the user's filesystem
    //
    // otherwise, returns an empty path
    std::optional<std::filesystem::path> TryFindInputFile(const OpenSim::Model&);

    // returns the absolute path to the given mesh component, if found (otherwise, std::nullptr)
    std::optional<std::filesystem::path> FindGeometryFileAbsPath(
        const OpenSim::Model&,
        const OpenSim::Mesh&
    );

    // returns the filename part of the `mesh_file` property (e.g. `C:\Users\adam\mesh.obj` returns `mesh.obj`)
    std::string GetMeshFileName(const OpenSim::Mesh&);

    // returns `true` if the component should be shown in the UI
    //
    // this uses heuristics to determine whether the component is something the UI should be
    // "revealed" to the user
    bool ShouldShowInUI(const OpenSim::Component&);

    // *tries* to delete the supplied component from the model
    //
    // returns `true` if the implementation was able to delete the component; otherwise, `false`
    bool TryDeleteComponentFromModel(OpenSim::Model&, OpenSim::Component&);

    // copy common joint properties from a `src` to `dest`
    //
    // e.g. names, coordinate names, etc.
    void CopyCommonJointProperties(const OpenSim::Joint& src, OpenSim::Joint& dest);

    // de-activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool DeactivateAllWrapObjectsIn(OpenSim::Model&);

    // activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool ActivateAllWrapObjectsIn(OpenSim::Model&);

    // returns pointers to all wrap objects that are referenced by the given `GeometryPath`
    std::vector<const OpenSim::WrapObject*> GetAllWrapObjectsReferencedBy(const OpenSim::GeometryPath&);

    // fully initialize an OpenSim model (clear connections, finalize properties, remake SimTK::System)
    void InitializeModel(OpenSim::Model&);

    // fully initalize an OpenSim model's working state
    SimTK::State& InitializeState(OpenSim::Model&);

    // calls `model.finalizeFromProperties()`
    //
    // (mostly here to match the style of osc's initialization methods)
    void FinalizeFromProperties(OpenSim::Model&);

    // finalize any socket connections in the model
    //
    // care:
    //
    // - it will _first_ finalize any properties in the model
    // - then it will finalize each socket recursively
    // - finalizing a socket causes the socket's pointer to write an updated
    //   component path to the socket's path property (for later serialization)
    void FinalizeConnections(OpenSim::Model&);

    // returns optional{index} if joint is found in parent jointset (otherwise: std::nullopt)
    std::optional<size_t> FindJointInParentJointSet(const OpenSim::Joint&);

    // returns user-visible (basic) name of geometry, or underlying file name
    std::string GetDisplayName(const OpenSim::Geometry&);

    // returns a user-visible string for a coordinate's motion type
    CStringView GetMotionTypeDisplayName(const OpenSim::Coordinate&);

    // returns a pointer to the component's appearance property, or `nullptr` if it doesn't have one
    const OpenSim::Appearance* TryGetAppearance(const OpenSim::Component&);
    OpenSim::Appearance* TryUpdAppearance(OpenSim::Component&);

    // tries to set the given component's appearance property's visibility field to the given bool
    //
    // returns `false` if the component doesn't have an appearance property, `true` if it does (and
    // the value was set)
    bool TrySetAppearancePropertyIsVisibleTo(OpenSim::Component&, bool);

    // returns the color part of the `OpenSim::Appearance` as an `osc::Color`
    Color to_color(const OpenSim::Appearance&);

    Color GetSuggestedBoneColor();  // best guess, based on shaders etc.

    // returns `true` if the given model's display properties asks to show frames
    bool IsShowingFrames(const OpenSim::Model&);

    // toggles the model's "show frames" display property and returns the new value
    bool ToggleShowingFrames(OpenSim::Model&);

    // returns `true` if the given model's display properties ask to show markers
    bool IsShowingMarkers(const OpenSim::Model&);

    // toggles the model's "show markers" display property and returns the new value
    bool ToggleShowingMarkers(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show wrap geometry
    bool IsShowingWrapGeometry(const OpenSim::Model&);

    // toggles the model's "show wrap geometry" display property and returns the new value
    bool ToggleShowingWrapGeometry(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show contact geometry
    bool IsShowingContactGeometry(const OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show forces
    bool IsShowingForces(const OpenSim::Model&);

    // toggles the model's "show contact geometry" display property and returns the new value
    bool ToggleShowingContactGeometry(OpenSim::Model&);

    // toggles the model's "show forces" display property and returns the new value
    bool ToggleShowingForces(OpenSim::Model&);

    // returns/assigns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePathString)
    void GetAbsolutePathString(const OpenSim::Component&, std::string&);
    std::string GetAbsolutePathString(const OpenSim::Component&);
    StringName GetAbsolutePathStringName(const OpenSim::Component&);

    // returns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath GetAbsolutePath(const OpenSim::Component&);

    // if non-nullptr, returns/assigns the absolute path to the component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath GetAbsolutePathOrEmpty(const OpenSim::Component*);

    // muscle lines of action
    //
    // helper functions for computing the "line of action" of a muscle. These algorithms were
    // adapted from: https://github.com/modenaxe/MuscleForceDirection/
    //
    // the reason they return `optional` is to handle edge-cases like the path containing an
    // insufficient number of points (shouldn't happen, but you never know)
    struct LinesOfAction final {
        PointDirection origin;
        PointDirection insertion;
    };
    std::optional<LinesOfAction> GetEffectiveLinesOfActionInGround(const OpenSim::Muscle&, const SimTK::State&);
    std::optional<LinesOfAction> GetAnatomicalLinesOfActionInGround(const OpenSim::Muscle&, const SimTK::State&);

    // returns a memory-safe equivalent to `OpenSim::GeometryPath::getPointForceDirections`
    std::vector<std::unique_ptr<OpenSim::PointForceDirection>> GetPointForceDirections(const OpenSim::GeometryPath&, const SimTK::State&);

    // path points
    //
    // helper functions for pulling path points out of geometry paths (e.g. for rendering)
    struct GeometryPathPoint final {

        explicit GeometryPathPoint(const Vec3& locationInGround_) :
            locationInGround{locationInGround_}
        {
        }

        GeometryPathPoint(
            const OpenSim::AbstractPathPoint& underlyingUserPathPoint,
            const Vec3& locationInGround_) :
            maybeUnderlyingUserPathPoint{&underlyingUserPathPoint},
            locationInGround{locationInGround_}
        {
        }

        const OpenSim::AbstractPathPoint* maybeUnderlyingUserPathPoint = nullptr;
        Vec3 locationInGround{};
    };
    std::vector<GeometryPathPoint> GetAllPathPoints(const OpenSim::GeometryPath&, const SimTK::State&);

    // contact forces
    //
    // helper functions for pulling contact forces out of the model (e.g. for rendering)
    struct ForcePoint final {
        Vec3 force;
        Vec3 point;
    };
    std::optional<ForcePoint> TryGetContactForceInGround(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::HuntCrossleyForce&
    );

    // force vectors
    //
    // helper functions for pulling force vectors out of components in the model
    const OpenSim::PhysicalFrame& GetFrameUsingExternalForceLookupHeuristic(
        const OpenSim::Model&,
        const std::string& bodyNameOrPath
    );

    // point info
    //
    // extract point-like information from generic OpenSim components
    struct PointInfo final {
        PointInfo(
            Vec3 location_,
            OpenSim::ComponentPath frameAbsPath_) :

            location{location_},
            frameAbsPath{std::move(frameAbsPath_)}
        {}

        Vec3 location;
        OpenSim::ComponentPath frameAbsPath;
    };
    bool CanExtractPointInfoFrom(const OpenSim::Component&, const SimTK::State&);
    std::optional<PointInfo> TryExtractPointInfo(const OpenSim::Component&, const SimTK::State&);

    // adds a component to an appropriate location in the model (e.g. jointset for a joint) and
    // returns a reference to the placed component
    OpenSim::Component& AddComponentToAppropriateSet(OpenSim::Model&, std::unique_ptr<OpenSim::Component>);

    // adds a model component to the componentset of a model and returns a reference to the component
    OpenSim::ModelComponent& AddModelComponent(OpenSim::Model&, std::unique_ptr<OpenSim::ModelComponent>);

    // adds a specific (T) model component to the componentset of the model and returns a reference to the component
    template<std::derived_from<OpenSim::ModelComponent> T>
    T& AddModelComponent(OpenSim::Model& model, std::unique_ptr<T> p)
    {
        return static_cast<T&>(AddModelComponent(model, static_cast<std::unique_ptr<OpenSim::ModelComponent>&&>(std::move(p))));
    }

    // constructs a specific (T) model component in the componentset of the model and returns a reference to the component
    template<std::derived_from<OpenSim::ModelComponent> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& AddModelComponent(OpenSim::Model& model, Args&&... args)
    {
        return AddModelComponent(model, std::make_unique<T>(std::forward<Args>(args)...));
    }

    // adds a new component to the componentset of the component and returns a reference to the new component
    OpenSim::Component& AddComponent(OpenSim::Component&, std::unique_ptr<OpenSim::Component>);

    template<std::derived_from<OpenSim::Component> T>
    T& AddComponent(OpenSim::Component& c, std::unique_ptr<T> p)
    {
        return static_cast<T&>(AddComponent(c, static_cast<std::unique_ptr<OpenSim::Component>&&>(std::move(p))));
    }

    template<std::derived_from<OpenSim::Component> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& AddComponent(OpenSim::Component& host, Args&&...args)
    {
        return AddComponent(host, std::make_unique<T>(std::forward<Args>(args)...));
    }

    OpenSim::Body& AddBody(OpenSim::Model&, std::unique_ptr<OpenSim::Body>);

    template<typename... Args>
    requires std::constructible_from<OpenSim::Body, Args&&...>
    OpenSim::Body& AddBody(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<OpenSim::Body>(std::forward<Args>(args)...);
        return AddBody(model, std::move(p));
    }

    OpenSim::Joint& AddJoint(OpenSim::Model&, std::unique_ptr<OpenSim::Joint>);

    template<std::derived_from<OpenSim::Joint> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& AddJoint(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(AddJoint(model, std::move(p)));
    }

    OpenSim::Marker& AddMarker(OpenSim::Model&, std::unique_ptr<OpenSim::Marker>);

    template<typename... Args>
    requires std::constructible_from<OpenSim::Marker, Args&&...>
    OpenSim::Marker& AddMarker(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<OpenSim::Marker>(std::forward<Args>(args)...);
        return AddMarker(model, std::move(p));
    }

    OpenSim::Geometry& AttachGeometry(OpenSim::Frame&, std::unique_ptr<OpenSim::Geometry>);

    template<std::derived_from<OpenSim::Geometry> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& AttachGeometry(OpenSim::Frame& frame, Args&&... args)
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(AttachGeometry(frame, std::move(p)));
    }

    OpenSim::PhysicalOffsetFrame& AddFrame(OpenSim::Joint&, std::unique_ptr<OpenSim::PhysicalOffsetFrame>);

    OpenSim::WrapObject& AddWrapObject(OpenSim::PhysicalFrame&, std::unique_ptr<OpenSim::WrapObject>);

    template<std::derived_from<OpenSim::WrapObject> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& AddWrapObject(OpenSim::PhysicalFrame& physFrame, Args&&... args)
    {
        return static_cast<T&>(AddWrapObject(physFrame, std::make_unique<T>(std::forward<Args>(args)...)));
    }

    template<ClonesToRawPointer T>
    std::unique_ptr<T> Clone(const T& obj)
    {
        return std::unique_ptr<T>(obj.clone());
    }

    template<std::derived_from<OpenSim::Component> T>
    void Append(OpenSim::ObjectProperty<T>& prop, const T& c)
    {
        prop.adoptAndAppendValue(Clone(c).release());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    std::optional<size_t> IndexOf(const OpenSim::Set<T, C>& set, const T& el)
    {
        for (size_t i = 0; i < size(set); ++i) {
            if (&At(set, i) == &el) {
                return i;
            }
        }
        return std::nullopt;
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    void Append(OpenSim::Set<T, C>& set, std::unique_ptr<U> el)
    {
        set.adoptAndAppend(el.release());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& Assign(OpenSim::Set<T, C>& set, size_t index, std::unique_ptr<U> el)
    {
        if (index >= size(set)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }

        U& rv = *el;
        set.set(static_cast<int>(index), el.release());
        return rv;
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& Assign(OpenSim::Set<T, C>& set, T& oldElement, std::unique_ptr<U> newElement)
    {
        auto idx = IndexOf(set, oldElement);
        if (not idx) {
            throw std::runtime_error{"cannot find the requested element in the set"};
        }
        return Assign(set, *idx, std::move(newElement));
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& Assign(OpenSim::Set<T, C>& set, size_t index, const U& el)
    {
        return Assign(set, index, Clone(el));
    }

    // tries to get the "parent" frame of the given component (if available)
    //
    // e.g. in OpenSim, this is usually acquired with `getParentFrame()`
    //      but that API isn't exposed generically via virtual methods
    const OpenSim::PhysicalFrame* TryGetParentToGroundFrame(const OpenSim::Component&);

    // tries to get the "parent" transform of the given component (if available)
    //
    // e.g. in OpenSim, this is usually acquired with `getParentFrame().getTransformInGround(State const&)`
    //      but that API isn't exposed generically via virtual methods
    std::optional<SimTK::Transform> TryGetParentToGroundTransform(const OpenSim::Component&, const SimTK::State&);

    // tries to get the name of the "positional" property of the given component
    //
    // e.g. the positional property of an `OpenSim::Station` is "location", whereas the
    //      positional property of an `OpenSim::PhysicalOffsetFrame` is "translation"
    std::optional<std::string> TryGetPositionalPropertyName(const OpenSim::Component&);

    // tries to get the name of the "orientational" property of the given component
    //
    // e.g. the orientational property of an `OpenSim::PhysicalOffsetFrame` is "orientation",
    //      whereas `OpenSim::Station` has no orientation, so this returns `std::nullopt`
    std::optional<std::string> TryGetOrientationalPropertyName(const OpenSim::Component&);

    // tries to return the "parent" of the given frame, if applicable (e.g. if the frame is an
    // `OffsetFrame<T>` that has a logical parent
    const OpenSim::Frame* TryGetParentFrame(const OpenSim::Frame&);

    // packages up the various useful parts that describe how a component is spatially represented
    //
    // see also (component functions):
    //
    // - TryGetParentToGroundTransform
    // - TryGetPositionalPropertyName
    // - TryGetOrientationalPropertyName
    struct ComponentSpatialRepresentation final {
        SimTK::Transform parentToGround;
        std::string locationVec3PropertyName;
        std::optional<std::string> maybeOrientationVec3EulersPropertyName;
    };

    // tries to get the "spatial" representation of a component
    std::optional<ComponentSpatialRepresentation> TryGetSpatialRepresentation(
        const OpenSim::Component&,
        const SimTK::State&
    );

    // returns `true` if the given character is permitted to appear within the name
    // of an `OpenSim::Component`
    bool IsValidOpenSimComponentNameCharacter(char);

    // returns a sanitized form of the given string that OpenSim would (probably) accept
    // as a component name
    std::string SanitizeToOpenSimComponentName(std::string_view);

    struct StorageLoadingParameters final {
        bool convertRotationalValuesToRadians = true;
        std::optional<double> resampleToFrequency = std::nullopt;
    };

    // returns an `OpenSim::Storage` with the given parameters
    //
    // (a `Model` is required because its `Coordinate`s are used to figure out which columns
    //  might be rotational)
    std::unique_ptr<OpenSim::Storage> LoadStorage(
        const OpenSim::Model&,
        const std::filesystem::path&,
        const StorageLoadingParameters& = {}
    );

    // Represents the result of trying to map columns in an `OpenSim::Storage` to state
    // variables in an `OpenSim::Model`.
    struct StorageIndexToModelStateVarMappingResult final {
        std::unordered_map<int, int> storageIndexToModelStatevarIndex;
        std::vector<std::string> stateVariablesMissingInStorage;
    };

    StorageIndexToModelStateVarMappingResult CreateStorageIndexToModelStatevarMapping(
        const OpenSim::Model&,
        const OpenSim::Storage&
    );

    std::unordered_map<int, int> CreateStorageIndexToModelStatevarMappingWithWarnings(
        const OpenSim::Model&,
        const OpenSim::Storage&
    );

    void UpdateStateVariablesFromStorageRow(
        OpenSim::Model&,
        SimTK::State&,
        const std::unordered_map<int, int>& columnIndexToModelStateVarIndex,
        const OpenSim::Storage&,
        int row
    );

    void UpdateStateFromStorageTime(
        OpenSim::Model&,
        SimTK::State&,
        const std::unordered_map<int, int>& columnIndexToModelStateVarIndex,
        const OpenSim::Storage&,
        double time
    );
}
