#pragma once

#include "src/OpenSimBindings/ComponentDecoration.hpp"

#include <OpenSim/Common/Component.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <string_view>

namespace OpenSim
{
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
    class ComponentPathPtrs {
        static constexpr size_t max_component_depth = 16;
        using container = std::array<OpenSim::Component const*, max_component_depth>;

        container els{};
        size_t n;

    public:
        explicit ComponentPathPtrs(OpenSim::Component const& c) : n{0}
        {
            OpenSim::Component const* cp = &c;

            els[n++] = cp;
            while (cp->hasOwner())
            {
                if (n >= max_component_depth)
                {
                    throw std::runtime_error{"cannot traverse hierarchy to a component: it is deeper than 32 levels in the component tree, which isn't currently supported by osc"};
                }

                cp = &cp->getOwner();
                els[n++] = cp;
            }
            std::reverse(els.begin(), els.begin() + n);
        }

        [[nodiscard]] constexpr container::const_iterator begin() const noexcept
        {
            return els.begin();
        }

        [[nodiscard]] constexpr container::const_iterator end() const noexcept
        {
            return els.begin() + n;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return n == 0;
        }
    };

    inline ComponentPathPtrs path_to(OpenSim::Component const& c)
    {
        return ComponentPathPtrs{c};
    }

    // returns the first ancestor of `c` that has type `T`
    template<typename T>
    T const* FindAncestorWithType(OpenSim::Component const* c)
    {
        while (c)
        {
            T const* p = dynamic_cast<T const*>(c);
            if (p)
            {
                return p;
            }
            c = c->hasOwner() ? &c->getOwner() : nullptr;
        }
        return nullptr;
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

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* FindComponentMut(OpenSim::Component& root, OpenSim::ComponentPath const&);

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<typename T>
    T const* FindComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
    {
        return dynamic_cast<T const*>(FindComponent(root, cp));
    }

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
}
