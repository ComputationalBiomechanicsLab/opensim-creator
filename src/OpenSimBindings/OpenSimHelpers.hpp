#pragma once

#include "src/OpenSimBindings/ComponentDecoration.hpp"

#include <filesystem>
#include <memory>
#include <vector>
#include <string_view>

namespace OpenSim
{
    class AbstractOutput;
    class Component;
    class ComponentPath;
    class AbstractSocket;
    class Model;
    class Coordinate;
    class Joint;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    int DistanceFromRoot(OpenSim::Component const&);

    OpenSim::ComponentPath const& GetEmptyComponentPath();

    std::vector<OpenSim::Component const*> GetPathElements(OpenSim::Component const&);

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

    void GetCoordinatesInModel(OpenSim::Model const&, std::vector<OpenSim::Coordinate const*>&);

    std::vector<OpenSim::AbstractSocket const*> GetAllSockets(OpenSim::Component&);
    std::vector<OpenSim::AbstractSocket const*> GetSocketsWithTypeName(OpenSim::Component& c, std::string_view);
    std::vector<OpenSim::AbstractSocket const*> GetPhysicalFrameSockets(OpenSim::Component& c);
    bool IsConnectedViaSocketTo(OpenSim::Component& c, OpenSim::Component const& other);
    bool IsAnyComponentConnectedViaSocketTo(OpenSim::Component& root, OpenSim::Component const& other);
    std::vector<OpenSim::Component*> GetAnyComponentsConnectedViaSocketTo(OpenSim::Component& root, OpenSim::Component const& other);

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

    template<typename T>
    T* FindComponentMut(OpenSim::Component& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T*>(FindComponentMut(root, cp));
    }

    OpenSim::AbstractOutput const* FindOutput(OpenSim::Component const&, std::string const& outputName);
    OpenSim::AbstractOutput const* FindOutput(OpenSim::Component const& root, OpenSim::ComponentPath const&, std::string const& outputName);

    // returns true if the path resolves to a component within root
    bool ContainsComponent(OpenSim::Component const& root, OpenSim::ComponentPath const&);

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
    void GenerateModelDecorations(OpenSim::Model const&,
                                  SimTK::State const&,
                                  float fixupScaleFactor,
                                  std::vector<osc::ComponentDecoration>&,
                                  OpenSim::Component const* selected,
                                  OpenSim::Component const* hovered);

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

    // initialize a model (finalized from properties, connected, system built, etc.)
    void InitalizeModel(OpenSim::Model&);

    // returns a model copy that is finalized from properties, connected, system built, etc.
    std::unique_ptr<OpenSim::Model> CreateInitializedModelCopy(OpenSim::Model const&);

    // adds a component to an appropriate (if possible - e.g. jointset) location in the model
    void AddComponentToModel(OpenSim::Model&, std::unique_ptr<OpenSim::Component>);
}
