#pragma once

#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/output_value.h>
#include <libopynsim/symbol.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    /// Represents a readonly multibody physics system compiled
    /// from a `ModelSpecification`.
    class Model final {
    private:
        friend class ModelSpecification;
        explicit Model(const OpenSim::Model&);

    public:
        /// Returns the initial (default) state of this model, which is
        /// the state that was defined by the `ModelSpecification` used
        /// to build this model.
        ModelState initial_state() const;

        /// Realize `model_state` to the given `model_state_stage`.
        void realize(ModelState& model_state, ModelStateStage model_state_stage) const;

        size_t num_coordinates() const;
        std::vector<Symbol> coordinates() const;
        double get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const;
        void   set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const;

        size_t num_outputs() const;
        std::vector<Symbol> outputs() const;
        OutputValue get_output_value(const ModelState& model_state, const Symbol& output) const;

        const OpenSim::Model& opensim_model() const;
    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
