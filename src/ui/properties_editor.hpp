#pragma once

#include <span-lite/nonstd/span.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace OpenSim {
    class Object;
    class AbstractProperty;
}

namespace osc::widgets::property_editor {
    struct State final {
        bool is_locked = true;
    };

    struct Response final {
        std::function<void(OpenSim::AbstractProperty&)> updater;
    };

    // if the user tries to edit the property, returns a function that performs
    // the equivalent mutation to the property
    std::optional<Response> draw(State&, OpenSim::AbstractProperty const& prop);
}

namespace osc::widgets::properties_editor {
    struct State final {
        std::vector<property_editor::State> property_editors;
    };

    struct Response final {
        OpenSim::AbstractProperty const& prop;
        std::function<void(OpenSim::AbstractProperty&)> updater;

        Response(OpenSim::AbstractProperty const& _prop, std::function<void(OpenSim::AbstractProperty&)> _updater) :
            prop{std::move(_prop)},
            updater{std::move(_updater)} {
        }
    };

    // if the user tries to edit one of the Object's properties, returns a
    // response that indicates which property was edited and a function that
    // performs an equivalent mutation to the property
    std::optional<Response> draw(State&, OpenSim::Object&);

    // as above, but only edit properties with the specified indices
    std::optional<Response> draw(State&, OpenSim::Object&, nonstd::span<int const> indices);
}
