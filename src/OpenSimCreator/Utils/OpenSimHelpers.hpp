#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PointDirection.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Concepts.hpp>

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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
namespace OpenSim { template<typename> class Property; }
namespace OpenSim { template<typename> class ObjectProperty; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { template<typename, typename> class Set; }
namespace OpenSim { template<typename> class SimpleProperty; }
namespace osc { class UndoableModelStatePair; }
namespace SimTK { class State; }

// OpenSimHelpers: a collection of various helper functions that are used by `osc`
namespace osc
{
    template<typename T>
    concept ClonesToRawPtr = requires(T const& v) {
        { v.clone() } -> std::same_as<T*>;
    };

    // iteration/indexing helpers

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    size_t size(OpenSim::Set<T, C> const& s)
    {
        return static_cast<size_t>(s.getSize());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    ptrdiff_t ssize(OpenSim::Set<T, C> const& s)
    {
        return static_cast<ptrdiff_t>(s.getSize());
    }

    template<typename T>
    size_t size(OpenSim::ArrayPtrs<T> const& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    template<typename T>
    size_t size(OpenSim::Array<T> const& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    template<typename T>
    size_t size(OpenSim::Property<T> const& p)
    {
        return static_cast<size_t>(p.size());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    [[nodiscard]] bool empty(OpenSim::Set<T, C> const& s)
    {
        return size(s) <= 0;
    }

    template<typename T>
    T& At(OpenSim::ArrayPtrs<T>& ary, size_t i)
    {
        if (i >= size(ary))
        {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }

        if (T* el = ary.get(static_cast<int>(i)))
        {
            return *el;
        }
        else
        {
            throw std::runtime_error{"attempted to access null element of ArrayPtrs"};
        }
    }

    template<typename T>
    T const& At(OpenSim::Array<T> const& ary, size_t i)
    {
        if (i >= size(ary))
        {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }
        return ary.get(static_cast<int>(i));
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    T const& At(OpenSim::Set<T, C> const& s, size_t i)
    {
        if (i >= size(s))
        {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        return s.get(static_cast<int>(i));
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    T& At(OpenSim::Set<T, C>& s, size_t i)
    {
        if (i >= size(s))
        {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        s.get(static_cast<int>(i));  // force an OpenSim-based null check
        return s[static_cast<int>(i)];
    }

    template<typename T>
    T const& At(OpenSim::Property<T> const& p, size_t i)
    {
        return p[static_cast<int>(i)];
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    bool EraseAt(OpenSim::Set<T, C>& s, size_t i)
    {
        return s.remove(static_cast<int>(i));
    }

    // returns true if the first argument has a lexographically lower class name
    bool IsConcreteClassNameLexographicallyLowerThan(
        OpenSim::Component const&,
        OpenSim::Component const&
    );

    // returns true if the first argument points to a component that has a lexographically lower class
    // name than the component pointed to by second argument
    //
    // (it's a helper method that's handy for use with pointers, unique_ptr, shared_ptr, etc.)
    template<DereferencesTo<OpenSim::Component const&> ComponentPtrLike>
    bool IsConcreteClassNameLexographicallyLowerThan(
        ComponentPtrLike const& a,
        ComponentPtrLike const& b)
    {
        return IsConcreteClassNameLexographicallyLowerThan(*a, *b);
    }

    bool IsNameLexographicallyLowerThan(
        OpenSim::Component const&,
        OpenSim::Component const&
    );

    template<DereferencesTo<OpenSim::Component const&> Ptr>
    bool IsNameLexographicallyLowerThan(
        Ptr const& a,
        Ptr const& b)
    {
        return IsNameLexographicallyLowerThan(*a, *b);
    }

    template<DereferencesTo<OpenSim::Component const&> Ptr>
    bool IsNameLexographicallyGreaterThan(
        Ptr const& a,
        Ptr const& b)
    {
        return !IsNameLexographicallyLowerThan<Ptr>(a, b);
    }

    // returns a mutable pointer to the owner (if it exists)
    OpenSim::Component* UpdOwner(OpenSim::Component& root, OpenSim::Component const&);

    template<std::derived_from<OpenSim::Component> T>
    T* UpdOwner(OpenSim::Component& root, OpenSim::Component const& c)
    {
        return dynamic_cast<T*>(UpdOwner(root, c));
    }

    // returns a pointer to the owner (if it exists)
    OpenSim::Component const& GetOwnerOrThrow(OpenSim::AbstractOutput const&);
    OpenSim::Component const& GetOwnerOrThrow(OpenSim::Component const&);
    OpenSim::Component const& GetOwnerOr(OpenSim::Component const&, OpenSim::Component const& fallback);
    OpenSim::Component const* GetOwner(OpenSim::Component const&);

    template<std::derived_from<OpenSim::Component> T>
    T const* GetOwner(OpenSim::Component const& c)
    {
        return dynamic_cast<T const*>(GetOwner(c));
    }

    template<std::derived_from<OpenSim::Component> T>
    bool OwnerIs(OpenSim::Component const& c)
    {
        return GetOwner<T>(c) != nullptr;
    }

    std::optional<std::string> TryGetOwnerName(OpenSim::Component const&);

    // returns the distance between the given `Component` and the component that is at the root of the component tree
    size_t DistanceFromRoot(OpenSim::Component const&);

    // returns a reference to a global instance of a path that points to the root of a model (i.e. "/")
    OpenSim::ComponentPath GetRootComponentPath();

    // returns `true` if the given `ComponentPath` is an empty path
    bool IsEmpty(OpenSim::ComponentPath const&);

    // clears the given component path
    void Clear(OpenSim::ComponentPath&);

    // returns all components between the root (element 0) and the given component (element n-1) inclusive
    std::vector<OpenSim::Component const*> GetPathElements(OpenSim::Component const&);

    // calls the given function with each subcomponent of the given component
    void ForEachComponent(OpenSim::Component const&, std::function<void(OpenSim::Component const&)> const&);

    template<std::derived_from<OpenSim::Component> T>
    size_t GetNumChildren(OpenSim::Component const& c)
    {
        size_t i = 0;
        ForEachComponent(c, [&i](OpenSim::Component const& c)
        {
            if (dynamic_cast<T const*>(&c))
            {
                ++i;
            }
        });
        return i;
    }

    // returns the number of direct children that the component owns
    size_t GetNumChildren(OpenSim::Component const&);

    // returns `true` if `c == parent` or `c` is a descendent of `parent`
    bool IsInclusiveChildOf(
        OpenSim::Component const* parent,
        OpenSim::Component const* c
    );

    // returns the first parent in `parents` that appears to be an inclusive parent of `c`
    //
    // returns `nullptr` if no element in `parents` is an inclusive parent of `c`
    OpenSim::Component const* IsInclusiveChildOf(
        std::span<OpenSim::Component const*> parents,
        OpenSim::Component const* c
    );

    // returns the first ancestor of `c` for which the given predicate returns `true`
    OpenSim::Component const* FindFirstAncestorInclusive(
        OpenSim::Component const*,
        bool(*pred)(OpenSim::Component const*)
    );

    // returns the first ancestor of `c` that has type `T`
    template<std::derived_from<OpenSim::Component> T>
    T const* FindAncestorWithType(OpenSim::Component const* c)
    {
        OpenSim::Component const* rv = FindFirstAncestorInclusive(c, [](OpenSim::Component const* el)
        {
            return dynamic_cast<T const*>(el) != nullptr;
        });

        return dynamic_cast<T const*>(rv);
    }

    // returns `true` if `c` is a child of a component that derives from `T`
    template<std::derived_from<OpenSim::Component> T>
    bool IsChildOfA(OpenSim::Component const& c)
    {
        return FindAncestorWithType<T>(&c) != nullptr;
    }

    // returns the first descendent (including `component`) that satisfies `predicate(descendent);`
    OpenSim::Component const* FindFirstDescendentInclusive(
        OpenSim::Component const& component,
        bool(*predicate)(OpenSim::Component const&)
    );

    // returns the first descendent of `component` that satisfies `predicate(descendent)`
    OpenSim::Component const* FindFirstDescendent(
        OpenSim::Component const& component,
        bool(*predicate)(OpenSim::Component const&)
    );

    // returns the first direct descendent of `component` that has type `T`, or
    // `nullptr` if no such descendent exists
    template<std::derived_from<OpenSim::Component> T>
    T const* FindFirstDescendentOfType(OpenSim::Component const& c)
    {
        OpenSim::Component const* rv = FindFirstDescendent(c, [](OpenSim::Component const& el)
        {
            return dynamic_cast<T const*>(&el) != nullptr;
        });
        return dynamic_cast<T const*>(rv);
    }

    // returns a vector containing points to all user-editable coordinates in the model
    std::vector<OpenSim::Coordinate const*> GetCoordinatesInModel(OpenSim::Model const&);

    // fills the given vector with all user-editable coordinates in the model
    void GetCoordinatesInModel(
        OpenSim::Model const&,
        std::vector<OpenSim::Coordinate const*>&
    );

    // returns the user-facing display value (i.e. degrees) for a coordinate
    float ConvertCoordValueToDisplayValue(OpenSim::Coordinate const&, double v);

    // returns the storage-facing value (i.e. radians) for a coordinate
    double ConvertCoordDisplayValueToStorageValue(OpenSim::Coordinate const&, float v);

    // returns a user-facing string that describes a coordinate's units
    CStringView GetCoordDisplayValueUnitsString(OpenSim::Coordinate const&);

    // returns the names of a component's sockets
    std::vector<std::string> GetSocketNames(OpenSim::Component const&);

    // returns all sockets that are directly attached to the given component
    std::vector<OpenSim::AbstractSocket const*> GetAllSockets(OpenSim::Component const&);

    // returns all (mutable) sockets that are directly attached to the given component
    std::vector<OpenSim::AbstractSocket*> UpdAllSockets(OpenSim::Component&);

    // returns a pointer if the given path resolves a component relative to root
    OpenSim::Component const* FindComponent(
        OpenSim::Component const& root,
        OpenSim::ComponentPath const&
    );
    OpenSim::Component const* FindComponent(
        OpenSim::Model const&,
        std::string const& absPath
    );

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    T const* FindComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T const*>(FindComponent(root, cp));
    }

    template<std::derived_from<OpenSim::Component> T>
    T const* FindComponent(OpenSim::Component const& root, std::string const& cp)
    {
        return dynamic_cast<T const*>(FindComponent(root, cp));
    }

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* FindComponentMut(
        OpenSim::Component& root,
        OpenSim::ComponentPath const&
    );

    // returns non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    T* FindComponentMut(
        OpenSim::Component& root,
        OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T*>(FindComponentMut(root, cp));
    }

    // returns true if the path resolves to a component within `root`
    bool ContainsComponent(
        OpenSim::Component const& root,
        OpenSim::ComponentPath const&
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    OpenSim::AbstractSocket const* FindSocket(
        OpenSim::Component const&,
        std::string const& socketName
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    OpenSim::AbstractSocket* FindSocketMut(
        OpenSim::Component&,
        std::string const& socketName
    );

    // returns `true` if the socket is connected to the component
    bool IsConnectedTo(
        OpenSim::AbstractSocket const&,
        OpenSim::Component const&
    );

    // returns true if the socket is able to connect to the component
    bool IsAbleToConnectTo(
        OpenSim::AbstractSocket const&,
        OpenSim::Component const&
    );

    // recursively traverses all components within `root` and reassigns any sockets
    // pointing to `from` to instead point to `to`
    //
    // note: must be called on a component that already has finalized connections
    void RecursivelyReassignAllSockets(
        OpenSim::Component& root,
        OpenSim::Component const& from,
        OpenSim::Component const& to
    );

    // returns a pointer to the property if the component has a property with the given name
    OpenSim::AbstractProperty* FindPropertyMut(
        OpenSim::Component&,
        std::string const&
    );

    // returns a pointer to the property if the component has a simple property with the given name and type
    template<typename T>
    OpenSim::SimpleProperty<T>* FindSimplePropertyMut(
        OpenSim::Component& c,
        std::string const& name)
    {
        return dynamic_cast<OpenSim::SimpleProperty<T>*>(FindPropertyMut(c, name));
    }

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to the given component
    OpenSim::AbstractOutput const* FindOutput(
        OpenSim::Component const&,
        std::string const& outputName
    );

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to a component located at the given path relative to the root
    OpenSim::AbstractOutput const* FindOutput(
        OpenSim::Component const& root,
        OpenSim::ComponentPath const&,
        std::string const& outputName
    );

    // returns true if the given model has an input file name (not empty, or "Unassigned")
    bool HasInputFileName(OpenSim::Model const&);

    // returns a non-empty path if the given model has an input file name that exists on the user's filesystem
    //
    // otherwise, returns an empty path
    std::filesystem::path TryFindInputFile(OpenSim::Model const&);

    // returns the absolute path to the given mesh component, if found (otherwise, std::nullptr)
    std::optional<std::filesystem::path> FindGeometryFileAbsPath(
        OpenSim::Model const&,
        OpenSim::Mesh const&
    );

    // returns `true` if the component should be shown in the UI
    //
    // this uses heuristics to determine whether the component is something the UI should be
    // "revealed" to the user
    bool ShouldShowInUI(OpenSim::Component const&);

    // *tries* to delete the supplied component from the model
    //
    // returns `true` if the implementation was able to delete the component; otherwise, `false`
    bool TryDeleteComponentFromModel(OpenSim::Model&, OpenSim::Component&);

    // copy common joint properties from a `src` to `dest`
    //
    // e.g. names, coordinate names, etc.
    void CopyCommonJointProperties(OpenSim::Joint const& src, OpenSim::Joint& dest);

    // de-activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool DeactivateAllWrapObjectsIn(OpenSim::Model&);

    // activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool ActivateAllWrapObjectsIn(OpenSim::Model&);

    // load an .osim file into an OpenSim model
    std::unique_ptr<UndoableModelStatePair> LoadOsimIntoUndoableModel(std::filesystem::path const&);

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
    std::optional<size_t> FindJointInParentJointSet(OpenSim::Joint const&);

    // returns a string representation of the recommended document's name
    std::string GetRecommendedDocumentName(UndoableModelStatePair const&);

    // returns user-visible (basic) name of geometry, or underlying file name
    std::string GetDisplayName(OpenSim::Geometry const&);

    // returns a user-visible string for a coordinate's motion type
    CStringView GetMotionTypeDisplayName(OpenSim::Coordinate const&);

    // returns a pointer to the component's appearance property, or `nullptr` if it doesn't have one
    OpenSim::Appearance const* TryGetAppearance(OpenSim::Component const&);
    OpenSim::Appearance* TryUpdAppearance(OpenSim::Component&);

    // tries to set the given component's appearance property's visibility field to the given bool
    //
    // returns `false` if the component doesn't have an appearance property, `true` if it does (and
    // the value was set)
    bool TrySetAppearancePropertyIsVisibleTo(OpenSim::Component&, bool);

    Color GetSuggestedBoneColor();  // best guess, based on shaders etc.

    // returns `true` if the given model's display properties asks to show frames
    bool IsShowingFrames(OpenSim::Model const&);

    // toggles the model's "show frames" display property and returns the new value
    bool ToggleShowingFrames(OpenSim::Model&);

    // returns `true` if the given model's display properties ask to show markers
    bool IsShowingMarkers(OpenSim::Model const&);

    // toggles the model's "show markers" display property and returns the new value
    bool ToggleShowingMarkers(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show wrap geometry
    bool IsShowingWrapGeometry(OpenSim::Model const&);

    // toggles the model's "show wrap geometry" display property and returns the new value
    bool ToggleShowingWrapGeometry(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show contact geometry
    bool IsShowingContactGeometry(OpenSim::Model const&);

    // toggles the model's "show contact geometry" display property and returns the new value
    bool ToggleShowingContactGeometry(OpenSim::Model&);

    // returns/assigns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePathString)
    void GetAbsolutePathString(OpenSim::Component const&, std::string&);
    std::string GetAbsolutePathString(OpenSim::Component const&);

    // returns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath GetAbsolutePath(OpenSim::Component const&);

    // if non-nullptr, returns/assigns the absolute path to the component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath GetAbsolutePathOrEmpty(OpenSim::Component const*);

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
    std::optional<LinesOfAction> GetEffectiveLinesOfActionInGround(OpenSim::Muscle const&, SimTK::State const&);
    std::optional<LinesOfAction> GetAnatomicalLinesOfActionInGround(OpenSim::Muscle const&, SimTK::State const&);

    // path points
    //
    // helper functions for pulling path points out of geometry paths (e.g. for rendering)
    struct GeometryPathPoint final {

        explicit GeometryPathPoint(Vec3 const& locationInGround_) :
            locationInGround{locationInGround_}
        {
        }

        GeometryPathPoint(
            OpenSim::AbstractPathPoint const& underlyingUserPathPoint,
            Vec3 const& locationInGround_) :
            maybeUnderlyingUserPathPoint{&underlyingUserPathPoint},
            locationInGround{locationInGround_}
        {
        }

        OpenSim::AbstractPathPoint const* maybeUnderlyingUserPathPoint = nullptr;
        Vec3 locationInGround{};
    };
    std::vector<GeometryPathPoint> GetAllPathPoints(OpenSim::GeometryPath const&, SimTK::State const&);

    // contact forces
    //
    // helper functions for pulling contact forces out of the model (e.g. for rendering)
    struct ForcePoint final {
        Vec3 force;
        Vec3 point;
    };
    std::optional<ForcePoint> TryGetContactForceInGround(
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::HuntCrossleyForce const&
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
        {
        }

        Vec3 location;
        OpenSim::ComponentPath frameAbsPath;
    };
    bool CanExtractPointInfoFrom(OpenSim::Component const&, SimTK::State const&);
    std::optional<PointInfo> TryExtractPointInfo(OpenSim::Component const&, SimTK::State const&);

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
    T& AddModelComponent(OpenSim::Model& model, Args&&... args)
        requires std::constructible_from<T, Args&&...>
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

    OpenSim::Body& AddBody(OpenSim::Model&, std::unique_ptr<OpenSim::Body>);
    OpenSim::Joint& AddJoint(OpenSim::Model&, std::unique_ptr<OpenSim::Joint>);
    OpenSim::Marker& AddMarker(OpenSim::Model&, std::unique_ptr<OpenSim::Marker>);

    template<typename... Args>
    OpenSim::Marker& AddMarker(OpenSim::Model& model, Args&&... args)
        requires std::constructible_from<OpenSim::Marker, Args&&...>
    {
        auto p = std::make_unique<OpenSim::Marker>(std::forward<Args>(args)...);
        return AddMarker(model, std::move(p));
    }

    OpenSim::Geometry& AttachGeometry(OpenSim::Frame&, std::unique_ptr<OpenSim::Geometry>);

    template<std::derived_from<OpenSim::Geometry> T, typename... Args>
    T& AttachGeometry(OpenSim::Frame& frame, Args&&... args)
        requires std::constructible_from<T, Args&&...>
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(AttachGeometry(frame, std::move(p)));
    }

    OpenSim::PhysicalOffsetFrame& AddFrame(OpenSim::Joint&, std::unique_ptr<OpenSim::PhysicalOffsetFrame>);

    template<ClonesToRawPtr T>
    std::unique_ptr<T> Clone(T const& obj)
    {
        return std::unique_ptr<T>(obj.clone());
    }

    template<std::derived_from<OpenSim::Component> T>
    void Append(OpenSim::ObjectProperty<T>& prop, T const& c)
    {
        prop.adoptAndAppendValue(Clone(c).release());
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
        if (index >= size(set))
        {
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
    U& Assign(OpenSim::Set<T, C>& set, size_t index, U const& el)
    {
        return Assign(set, index, Clone(el));
    }

    // tries to get the "parent" transform of the given component (if available)
    //
    // e.g. in OpenSim, this is usually acquired with `getParentFrame().getTransformInGround(State const&)`
    //      but that API isn't exposed generically via virtual methods
    std::optional<SimTK::Transform> TryGetParentToGroundTransform(OpenSim::Component const&, SimTK::State const&);

    // tries to get the name of the "positional" property of the given component
    //
    // e.g. the positional property of an `OpenSim::Station` is "location", whereas the
    //      positional property of an `OpenSim::PhysicalOffsetFrame` is "translation"
    std::optional<std::string> TryGetPositionalPropertyName(OpenSim::Component const&);

    // tries to get the name of the "orientational" property of the given component
    //
    // e.g. the orientational property of an `OpenSim::PhysicalOffsetFrame` is "orientation",
    //      whereas `OpenSim::Station` has no orientation, so this returns `std::nullopt`
    std::optional<std::string> TryGetOrientationalPropertyName(OpenSim::Component const&);

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
        OpenSim::Component const&,
        SimTK::State const&
    );

    // returns `true` if the given character is permitted to appear within the name
    // of an `OpenSim::Component`
    bool IsValidOpenSimComponentNameCharacter(char);

    // returns a sanitized form of the given string that OpenSim would (probably) accept
    // as a component name
    std::string SanitizeToOpenSimComponentName(std::string_view);
}
