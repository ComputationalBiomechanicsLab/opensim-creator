#pragma once

#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/model_states.h>
#include <libopynsim/output_value.h>
#include <libopynsim/symbol.h>

#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <string>
#include <vector>

namespace osc { class SceneCache; }
namespace OpenSim { class Model; }
namespace opyn { class DataFrame; }

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

        /// Returns the names of the columns in `data_frame` that can
        /// be mapped to rotational state variables in this `Model` in
        /// the column-order of `data_frame`.
        std::vector<std::string> rotational_columns_in(const DataFrame& data_frame) const;

        /// Returns associative mappings between the names of columns in
        /// `data_frame` and state variables in this `Model`, where the
        /// correspondence can be found.
        std::unordered_map<std::string, Symbol> column_to_state_variable_mappings(const DataFrame& data_frame) const;

        /// Returns `ModelStates` constructed from `data_frame`.
        ///
        /// Columns in `data_frame` will be mapped to state variables in this
        /// `Model` (see: `column_to_state_variable_mappings`). Each row in
        /// `data_frame` constructs one state in the returned `ModelStates`, in
        /// row-order.
        ///
        /// If `data_frame.attrs()["inDegrees"] == "yes"` then rotational columns in
        /// `data_frame` will be automatically converted into radians internally
        /// (see: `rotational_columns_in`). This is to support `DataFrame`s loaded
        /// from legacy data sources.
        ModelStates states_from_data_frame(const DataFrame& data_frame) const;

        /// Realizes `model_state` to the given `model_state_stage`.
        void realize(ModelState& model_state, ModelStateStage model_state_stage) const;

        size_t num_coordinates() const;
        std::vector<Symbol> coordinates() const;
        double get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const;
        void   set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const;

        size_t num_outputs() const;
        std::vector<Symbol> outputs() const;
        OutputValue get_output_value(const ModelState& model_state, const Symbol& output) const;

        std::vector<osc::SceneDecoration> decorations(
            osc::SceneCache&,
            const ModelState&,
            const OpenSimDecorationOptions& = {}
        ) const;

    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
