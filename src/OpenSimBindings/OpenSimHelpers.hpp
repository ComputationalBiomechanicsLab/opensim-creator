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

    std::vector<OpenSim::AbstractSocket const*> GetAllSockets(OpenSim::Component&);
    std::vector<OpenSim::AbstractSocket const*> GetSocketsWithTypeName(OpenSim::Component& c, std::string_view);
    std::vector<OpenSim::AbstractSocket const*> GetPhysicalFrameSockets(OpenSim::Component& c);

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

    // generates decorations for a model + state
    void GenerateModelDecorations(OpenSim::Model const&,
                                  SimTK::State const&,
                                  float fixupScaleFactor,
                                  std::vector<osc::ComponentDecoration>&,
                                  OpenSim::Component const* selected,
                                  OpenSim::Component const* hovered);

    void UpdateSceneBVH(nonstd::span<ComponentDecoration const>, BVH&);
}
