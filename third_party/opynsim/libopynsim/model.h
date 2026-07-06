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
#include <unordered_map>
#include <unordered_set>
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
        ///
        /// The `realized_to` argument ensures the returned `ModelState`
        /// is realized as-if by calling `this->realize(returned_state, realized_to)`.
        ModelState initial_state(ModelStateStage realized_to = ModelStateStage::instance) const;

        /// Returns a set of the names of the columns in `data_frame` that can
        /// be mapped to rotational state variables in this `Model`.
        std::unordered_set<std::string> rotational_columns_in(const DataFrame& data_frame) const;

        /// Returns associative mappings between the names of columns in
        /// `data_frame` and state variables in this `Model`, where the
        /// correspondence can be found.
        std::unordered_map<std::string, Symbol> column_to_state_variable_mappings(const DataFrame& data_frame) const;

        /// Returns a new `DataFrame` that's the result of converting rotational columns
        /// in `data_frame` to radians (if applicable).
        ///
        /// Behavior:
        /// - If `data_frame.attrs()["inDegrees"] != "yes"`, returns a
        ///   copy of `data_frame` as-is.
        /// - Otherwise, copies `data_frame` and uses `this->rotational_columns_in(copy)`
        ///   to figure out which columns in are rotational, followed by scaling the
        ///   entire column to radians. The `"inDegrees"` key is cleared from the
        ///   returned copy.
        DataFrame convert_data_frame_to_radians(const DataFrame& data_frame) const;

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
        ///
        /// The `realized_to` argument ensures each returned `ModelState`s is realized
        /// as-if by calling `this->realize(returned_states[i], realized_to)`.
        ModelStates states_from_data_frame(
            const DataFrame& data_frame,
            ModelStateStage realized_to = ModelStateStage::instance
        ) const;

        /// Realizes `model_state` to the given `model_state_stage`.
        void realize(ModelState& model_state, ModelStateStage model_state_stage) const;

        /// Returns the number of coordinates in the model.
        size_t num_coordinates() const;

        /// Returns all coordinates in the model.
        std::vector<Symbol> coordinates() const;

        /// Returns the value of `coordinate` in `model_state`.
        double get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const;

        /// Sets the value of `coordinate` in `model_state` to `value`.
        void set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const;

        /// Returns the speed of `coordinate` in `model_state`.
        double get_coordinate_speed(const ModelState& model_state, const Symbol& coordinate) const;

        /// Sets the speed of `coordinate` in `model_state` to `speed`.
        void set_coordinate_speed(ModelState& model_state, const Symbol& coordinate, double speed) const;

        /// Returns `true` if `coordinate` is locked in `model_state`.
        bool is_coordinate_locked(const ModelState& model_state, const Symbol& coordinate) const;

        /// Sets the locked state of `coordinate` in `model_state` to `locked`.
        void set_coordinate_locked(ModelState& model_state, const Symbol& coordinate, bool locked = true) const;

        /// Returns `true` if `coordinate` has a rotational (i.e. not translational or coupled).
        bool is_coordinate_rotational(const Symbol& coordinate) const;

        /// Returns the number of outputs in the model.
        size_t num_outputs() const;

        /// Returns all outputs in the model.
        std::vector<Symbol> outputs() const;

        /// Returns the value of `output` in `model_state`.
        OutputValue get_output_value(const ModelState& model_state, const Symbol& output) const;

        /// Returns scene decorations that visually represent `*this` in `model_state`.
        std::vector<osc::SceneDecoration> decorations(
            osc::SceneCache&,
            const ModelState& model_state,
            const OpenSimDecorationOptions& = {}
        ) const;

        /// Returns the underlying `OpenSim::Model` (internal method: be careful with this).
        const OpenSim::Model& open_sim_model() const;
    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
