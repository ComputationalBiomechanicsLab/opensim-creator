#pragma once

#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/CustomDecorationOptions.hpp"
#include "src/Utils/CStringView.hpp"

#include <filesystem>
#include <memory>
#include <vector>
#include <string_view>

namespace OpenSim
{
    class AbstractOutput;
    class AbstractSocket;
    class Component;
    class ComponentPath;
    class Coordinate;
    class Geometry;
    class Joint;
    class Model;
}

namespace osc
{
    class VirtualConstModelStatePair;
    class UndoableModelStatePair;
}

namespace SimTK
{
    class State;
}

// OpenSimHelpers: a collection of various helper functions that are used by `osc`
namespace osc
{
    // returns the distance between the given `Component` and the component that is at the root of the component tree
    int DistanceFromRoot(OpenSim::Component const&);

    // returns a global instance of an empty component path
    OpenSim::ComponentPath const& GetEmptyComponentPath();

    // returns `true` if the given `ComponentPath` is an empty path
    bool IsEmpty(OpenSim::ComponentPath const&);

    // clears the given component path
    void Clear(OpenSim::ComponentPath&);

    // returns all components between the root (element 0) and the given component (element n-1) inclusive
    std::vector<OpenSim::Component const*> GetPathElements(OpenSim::Component const&);

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

    // fills the given vector with all user-editable coordinates in the model
    void GetCoordinatesInModel(OpenSim::Model const&, std::vector<OpenSim::Coordinate const*>&);

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
    OpenSim::Component const* FindComponent(OpenSim::Component const& root, OpenSim::ComponentPath const&);

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<typename T>
    T const* FindComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T const*>(FindComponent(root, cp));
    }

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* FindComponentMut(OpenSim::Component& root, OpenSim::ComponentPath const&);

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<typename T>
    T* FindComponentMut(OpenSim::Component& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T*>(FindComponentMut(root, cp));
    }

    // returns true if the path resolves to a component within root
    bool ContainsComponent(OpenSim::Component const& root, OpenSim::ComponentPath const&);

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to the given component
    OpenSim::AbstractOutput const* FindOutput(OpenSim::Component const&, std::string const& outputName);

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to a component located at the given path relative to the root
    OpenSim::AbstractOutput const* FindOutput(OpenSim::Component const& root, OpenSim::ComponentPath const&, std::string const& outputName);

    // returns true if the given model has an input file name (not empty, or "Unassigned")
    bool HasInputFileName(OpenSim::Model const&);

    // returns a non-empty path if the given model has an input file name that exists on the user's filesystem
    //
    // otherwise, returns an empty path
    std::filesystem::path TryFindInputFile(OpenSim::Model const&);

    // returns `true` if the component should be shown in the UI
    //
    // this uses heuristics to determine whether the component is something the UI should be
    // "revealed" to the user
    bool ShouldShowInUI(OpenSim::Component const&);

    // *tries* to delete the supplied component from the model
    //
    // returns `true` if the implementation was able to delete the component; otherwise, `false`
    bool TryDeleteComponentFromModel(OpenSim::Model&, OpenSim::Component&);

    // generates decorations for a model + state
    void GenerateModelDecorations(VirtualConstModelStatePair const&, std::vector<osc::ComponentDecoration>&, CustomDecorationOptions = {});

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(nonstd::span<ComponentDecoration const>, BVH&);

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

    // returns the recommended scale factor for the given model+state pair
    float GetRecommendedScaleFactor(VirtualConstModelStatePair const&);

    // load an .osim file into an OpenSim model
    std::unique_ptr<UndoableModelStatePair> LoadOsimIntoUndoableModel(std::filesystem::path);

    // fully initialize an OpenSim model (from properties, remake SimTK::System, etc.)
    SimTK::State& Initialize(OpenSim::Model&);

    // returns -1 if joint isn't in a set or cannot be found
    int FindJointInParentJointSet(OpenSim::Joint const&);

    // returns a string representation of the recommended document's name
    std::string GetRecommendedDocumentName(osc::UndoableModelStatePair const&);

    // returns user-visible (basic) name of geometry, or underlying file name
    std::string GetDisplayName(OpenSim::Geometry const&);

    // returns a user-visible string for a coordinate's motion type
    char const* GetMotionTypeDisplayName(OpenSim::Coordinate const&);
}
