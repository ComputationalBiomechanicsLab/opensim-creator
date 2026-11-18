#pragma once

#include <libopynsim/model_state_stage.h>
#include <libopynsim/symbol.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <format>
#include <unordered_map>

namespace SimTK
{
    class State;
}

namespace opyn
{
    class Model;

    // Represents all state information for a `Model`.
    //
    // Related: https://simtk.org/api_docs/simbody/3.5/classSimTK_1_1State.html#details
    // Related:
    // https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089017/SimTK+Simulation+Concepts
    class ModelState final {
    public:
        using attrs_type = std::unordered_map<Symbol, double>;

        /// Constructs an empty model state.
        ModelState();

        explicit ModelState(SimTK::State&&);

        /// Returns the `ModelStateStage` that this state has been realized to.
        ModelStateStage stage() const;

        /// Returns the time point, in seconds, that this state represents.
        double time() const;

        /// Returns this `ModelState`'s metadata (e.g. integrator stats, performance counters).
        const attrs_type& attrs() const;

        /// Sets the `attrs` of this `ModelState` to `new_attrs`.
        void set_attrs(attrs_type new_attrs);

        /// Returns the underlying `SimTK::State` (internal method: be careful with this).
        const SimTK::State& simbody_state() const;

        /// Returns the underlying `SimTK::State` (internal method: be careful with this).
        SimTK::State& upd_simbody_state();

    private:
        class Impl;
        friend bool operator==(const ModelState&, const ModelState&);

        osc::CopyOnUpdPtr<Impl> impl_;
    };

    /// Returns `true` if `lhs` and `rhs` are memory-equivalent at the
    /// time/stage/state-variable level.
    bool operator==(const ModelState& lhs, const ModelState& rhs);
}

template<>
struct std::formatter<opyn::ModelState> {
    constexpr auto parse(std::format_parse_context& ctx) const { return ctx.begin(); }

    template<class FormatContext>
    auto format(const opyn::ModelState& model_state, FormatContext& ctx) const
    {
        return std::format_to(
            ctx.out(), "ModelState(time={}, stage={})", model_state.time(), model_state.stage());
    }
};
