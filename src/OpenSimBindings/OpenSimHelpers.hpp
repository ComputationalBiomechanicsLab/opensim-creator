#pragma once

#include "src/Maths/Plane.hpp"
#include "src/Maths/PointDirection.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>
#include <OpenSim/Common/ComponentPath.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class AbstractPathPoint; }
namespace OpenSim { class AbstractProperty; }
namespace OpenSim { class AbstractSocket; }
namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class GeometryPath; }
namespace OpenSim { class HuntCrossleyForce; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class Muscle; }
namespace osc { class UndoableModelStatePair; }
namespace SimTK { class State; }

// OpenSimHelpers: a collection of various helper functions that are used by `osc`
namespace osc
{
    // returns true if the first argument has a lexographically lower class name
    bool IsConcreteClassNameLexographicallyLowerThan(
        OpenSim::Component const&,
        OpenSim::Component const&
    );

    // returns true if the first argument points to a component that has a lexographically lower class
    // name than the component pointed to by second argument
    //
    // (it's a helper method that's handy for use with pointers, unique_ptr, shared_ptr, etc.)
    template<typename ComponentPtrLike>
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

    template<typename ComponentPtrLike>
    bool IsNameLexographicallyLowerThan(
        ComponentPtrLike const& a,
        ComponentPtrLike const& b)
    {
        return IsNameLexographicallyLowerThan(*a, *b);
    }

    template<typename ComponentPtrLike>
    bool IsNameLexographicallyGreaterThan(
        ComponentPtrLike const& a,
        ComponentPtrLike const& b)
    {
        return !IsNameLexographicallyLowerThan<ComponentPtrLike>(a, b);
    }

    // returns a mutable pointer to the owner (if it exists)
    OpenSim::Component* UpdOwner(OpenSim::Component&);
    template<typename T>
    T* UpdOwner(OpenSim::Component& c)
    {
        return dynamic_cast<T*>(UpdOwner(c));
    }

    // returns a pointer to the owner (if it exists)
    OpenSim::Component const* GetOwner(OpenSim::Component const&);
    template<typename T>
    T const* GetOwner(OpenSim::Component const& c)
    {
        return dynamic_cast<T const*>(GetOwner(c));
    }

    // returns the distance between the given `Component` and the component that is at the root of the component tree
    int DistanceFromRoot(OpenSim::Component const&);

    // returns a reference to a global instance of an empty component path (i.e. "")
    OpenSim::ComponentPath const& GetEmptyComponentPath();

    // returns a reference to a global instance of a path that points to the root of a model (i.e. "/")
    OpenSim::ComponentPath const& GetRootComponentPath();

    // returns `true` if the given `ComponentPath` is an empty path
    bool IsEmpty(OpenSim::ComponentPath const&);

    // clears the given component path
    void Clear(OpenSim::ComponentPath&);

    // returns all components between the root (element 0) and the given component (element n-1) inclusive
    std::vector<OpenSim::Component const*> GetPathElements(OpenSim::Component const&);

    // returns `true` if `c == parent` or `c` is a descendent of `parent`
    bool IsInclusiveChildOf(
        OpenSim::Component const* parent,
        OpenSim::Component const* c
    );

    // returns the first parent in `parents` that appears to be an inclusive parent of `c`
    //
    // returns `nullptr` if no element in `parents` is an inclusive parent of `c`
    OpenSim::Component const* IsInclusiveChildOf(
        nonstd::span<OpenSim::Component const*> parents,
        OpenSim::Component const* c
    );

    // returns the first ancestor of `c` for which the given predicate returns `true`
    OpenSim::Component const* FindFirstAncestorInclusive(OpenSim::Component const*, bool(*pred)(OpenSim::Component const*));

    // returns the first ancestor of `c` that has type `T`
    template<typename T>
    T const* FindAncestorWithType(OpenSim::Component const* c)
    {
        OpenSim::Component const* rv = FindFirstAncestorInclusive(c, [](OpenSim::Component const* el)
        {
                return static_cast<bool>(dynamic_cast<T const*>(el));
        });

        return static_cast<T const*>(rv);
    }

    template<typename T>
    T* FindAncestorWithTypeMut(OpenSim::Component* c)
    {
        return const_cast<T*>(FindAncestorWithType<T>(c));
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
    template<typename T>
    T const* FindComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T const*>(FindComponent(root, cp));
    }

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* FindComponentMut(
        OpenSim::Component& root,
        OpenSim::ComponentPath const&
    );

    // returns non-nullptr if the given path resolves a component of type T relative to root
    template<typename T>
    T* FindComponentMut(
        OpenSim::Component& root,
        OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T*>(FindComponentMut(root, cp));
    }

    // returns true if the path resolves to a component within root
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

    // returns true if the socket is able to connect to the component
    bool IsAbleToConnectTo(
        OpenSim::AbstractSocket const&,
        OpenSim::Component const&
    );

    // returns a pointer to the property if the component has a property with the given name
    OpenSim::AbstractProperty* FindPropertyMut(
        OpenSim::Component&,
        std::string const&
    );

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

    // adds a component to an appropriate (if possible - e.g. jointset) location in the model
    void AddComponentToModel(OpenSim::Model&, std::unique_ptr<OpenSim::Component>);

    // load an .osim file into an OpenSim model
    std::unique_ptr<UndoableModelStatePair> LoadOsimIntoUndoableModel(std::filesystem::path const&);

    // fully initialize an OpenSim model (clear connections, finalize properties, remake SimTK::System)
    void InitializeModel(OpenSim::Model&);

    // fully initalize an OpenSim model's working state
    SimTK::State& InitializeState(OpenSim::Model&);

    // returns optional{index} if joint is found in parent jointset (otherwise: std::nullopt)
    std::optional<int> FindJointInParentJointSet(OpenSim::Joint const&);

    // returns a string representation of the recommended document's name
    std::string GetRecommendedDocumentName(osc::UndoableModelStatePair const&);

    // returns user-visible (basic) name of geometry, or underlying file name
    std::string GetDisplayName(OpenSim::Geometry const&);

    // returns a user-visible string for a coordinate's motion type
    CStringView GetMotionTypeDisplayName(OpenSim::Coordinate const&);

    // tries to set the given component's appearance property's visibility field to the given bool
    //
    // returns `false` if the component doesn't have an appearance property, `true` if it does (and
    // the value was set)
    bool TrySetAppearancePropertyIsVisibleTo(OpenSim::Component&, bool);

    glm::vec4 GetSuggestedBoneColor() noexcept;  // best guess, based on shaders etc.

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

        explicit GeometryPathPoint(glm::vec3 const& locationInGround_) :
            locationInGround{locationInGround_}
        {
        }

        GeometryPathPoint(
            OpenSim::AbstractPathPoint const& underlyingUserPathPoint,
            glm::vec3 const& locationInGround_) :
            maybeUnderlyingUserPathPoint{&underlyingUserPathPoint},
            locationInGround{locationInGround_}
        {
        }

        OpenSim::AbstractPathPoint const* maybeUnderlyingUserPathPoint = nullptr;
        glm::vec3 locationInGround{};
    };
    std::vector<GeometryPathPoint> GetAllPathPoints(OpenSim::GeometryPath const&, SimTK::State const&);

    // contact forces
    //
    // helper functions for pulling contact forces out of the model (e.g. for rendering)
    struct ForcePoint final {
        glm::vec3 force;
        glm::vec3 point;
    };
    std::optional<ForcePoint> TryGetContactForceInGround(
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::HuntCrossleyForce const&
    );
}
