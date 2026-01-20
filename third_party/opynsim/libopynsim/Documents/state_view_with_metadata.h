#pragma once

#include <liboscar/utils/uid.h>

#include <optional>

namespace SimTK { class State; }

namespace osc
{
    // An abstract base class that represents the pairing of a readonly `SimTK::State` with metadata.
    class StateViewWithMetadata {
    protected:
        StateViewWithMetadata() = default;
        StateViewWithMetadata(const StateViewWithMetadata&) = default;
        StateViewWithMetadata(StateViewWithMetadata&&) noexcept = default;
        ~StateViewWithMetadata() noexcept = default;

        StateViewWithMetadata& operator=(const StateViewWithMetadata&) = default;
        StateViewWithMetadata& operator=(StateViewWithMetadata&&) noexcept = default;
        friend bool operator==(const StateViewWithMetadata&, const StateViewWithMetadata&) = default;

    public:
        // Returns a reference to the readonly `SimTK::State` this view is viewing.
        const SimTK::State& getState() const { return implGetState(); }

        // Returns a single auxiliary value (metadata) associated with the state that this
        // view is viewing.
        std::optional<float> getAuxiliaryValue(UID id) const { return implGetAuxiliaryValue(id); }

    private:
        // Implementors must provide a const accessor to a state
        virtual const SimTK::State& implGetState() const = 0;

        // Implementors may provide a way of accessing auxiliary (metadata)
        // associated with the state.
        virtual std::optional<float> implGetAuxiliaryValue(UID) const { return std::nullopt; }
    };
}
