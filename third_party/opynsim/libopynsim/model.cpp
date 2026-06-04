#include "model.h"

#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/data_frame.h>
#include <libopynsim/model_state.h>
#include <libopynsim/output_value.h>

#include <ankerl/unordered_dense.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/typelist.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    template<typename OPynSimType>
    struct OutputTypeMapping;

    template<>
    struct OutputTypeMapping<double> {
        using OpenSimType = double;

        static OutputValue extract_output_value(const SimTK::State& state, const OpenSim::AbstractOutput& output)
        {
            const auto& opensim_output = dynamic_cast<const OpenSim::Output<OpenSimType>&>(output);
            return opensim_output.getValue(state);
        }
    };

    template<>
    struct OutputTypeMapping<osc::Vector3d> {
        using OpenSimType = SimTK::Vec3;

        static OutputValue extract_output_value(const SimTK::State& state, const OpenSim::AbstractOutput& output)
        {
            const auto& opensim_output = dynamic_cast<const OpenSim::Output<OpenSimType>&>(output);
            return osc::to<osc::Vector3d>(opensim_output.getValue(state));
        }
    };

    // Represents all supported output extractors.
    class OutputExtractionSystem final {
    public:
        // Represents a type-erased runtime output extraction function.
        using OutputValueExtractor = OutputValue (*)(const SimTK::State&, const OpenSim::AbstractOutput&);

        // Returns `true` if `output` is compatible with the OPynSim output extraction system.
        bool is_compatible_with(const OpenSim::AbstractOutput& output) const
        {
            return osc::cpp23::contains(opensim_type_indices_, std::type_index{typeid(output)});
        }

        // Reads an `OutputValue` from `model_state` for the given `output`.
        OutputValue read_value(const SimTK::State& state, const OpenSim::AbstractOutput& output) const
        {
            const auto it = rgs::find(opensim_type_indices_, std::type_index{typeid(output)});
            if (it == opensim_type_indices_.end()) {
                std::stringstream ss;
                ss << "Could not find a suitable output extractor for " << output.getName() << ": this is an engine error (it shouldn't have shown you it!)";
                throw std::runtime_error{std::move(ss).str()};
            }
            static_assert(std::tuple_size_v<decltype(opensim_type_indices_)> == std::tuple_size_v<decltype(value_extractors_)>);
            const auto& value_extractor = value_extractors_[std::distance(opensim_type_indices_.begin(), it)];
            return value_extractor(state, output);
        }
    private:
        template<typename... Ts>
        static std::array<std::type_index, osc::TypelistSizeV<SupportedOutputValueTypes>> output_type_indexes(osc::Typelist<Ts...>)
        {
            return std::to_array({ std::type_index(typeid(OpenSim::Output<typename OutputTypeMapping<Ts>::OpenSimType>))... });
        }

        // Helper function that generates a dense LUT that maps the index of an output
        // type to an output extraction function.
        template<typename... Ts>
        static std::array<OutputValueExtractor, osc::TypelistSizeV<SupportedOutputValueTypes>> output_value_extractors(osc::Typelist<Ts...>)
        {
            return std::to_array<OutputValueExtractor>({ OutputTypeMapping<Ts>::extract_output_value... });
        }

        std::array<std::type_index,      osc::TypelistSizeV<SupportedOutputValueTypes>> opensim_type_indices_ = output_type_indexes(SupportedOutputValueTypes{});
        std::array<OutputValueExtractor, osc::TypelistSizeV<SupportedOutputValueTypes>> value_extractors_ = output_value_extractors(SupportedOutputValueTypes{});
    };

    // Returns a constant reference to a global `OutputExtractionSystem` with static lifetime.
    const OutputExtractionSystem& output_extraction_system()
    {
        static const OutputExtractionSystem s_output_extraction_system;
        return s_output_extraction_system;
    }

    // Returns an encoded (symbolic) representation of a `(Component, AbstractOutput)` pair.
    Symbol encode_component_output_into_symbol(
        std::string_view component_abs_path,
        const OpenSim::AbstractOutput& output)
    {
        std::string rv;
        rv.reserve(component_abs_path.size() + 1 + output.getName().size() + 1);
        rv += component_abs_path;
        rv += '[';
        rv += output.getName();
        rv += ']';
        return Symbol{std::move(rv)};
    }

    // Returns an O(1) Symbol --> Output* lookup that's populated ahead-of-time so that
    // the API can rapidly find+retrieve outputs from a `Model`.
    ankerl::unordered_dense::map<Symbol, const OpenSim::AbstractOutput*> collect_model_outputs(const OpenSim::Model& model)
    {
        const auto& output_system = output_extraction_system();

        ankerl::unordered_dense::map<Symbol, const OpenSim::AbstractOutput*> rv;
        for (const auto& component : model.getComponentList()) {
            const std::string abs_path = component.getAbsolutePathString();
            rv.reserve(rv.size() + component.getNumOutputs());
            for (const auto& [name, output] : component.getOutputs()) {
                if (output_system.is_compatible_with(*output)) {
                    rv.try_emplace(encode_component_output_into_symbol(abs_path, *output), output.get());
                }
            }
        }
        return rv;
    }

    OpenSim::Model construct_built_model(const OpenSim::Model& specification_model)
    {
        // Construct a copy (properties etc.) of the specification model.
        OpenSim::Model rv{specification_model};

        // This is effectively what converting a "model specification" to
        // a "model" is, in `opynsim`'s world.
        rv.buildSystem();

        // This is a quirk of OpenSim, because it mixes the state and system
        // into one class, but it must be done here because `initial_state()`
        // is `const` in `opynsim`'s design.
        rv.initializeState();

        return rv;
    }

    const OpenSim::Coordinate* find_rotational_coordinate_matching_name(
        const OpenSim::Model& model,
        std::string_view name)
    {
        // Matching has some legacy behaviors (see `OpenSim::SimbodyEngine::scaleRotationDofColumns`). The
        // following names from data columns must be tested:
        //
        // - `${name}`                             Flat name, used in OpenSim 3.x (directly maps to a state variable/coordinate)
        // - `${name}_u`                           Flat name for coordinate speed, used in OpenSim 3.x
        // - `${name}_vel`                         Flat name for coordinate speed, used in SIMM and OpenSim 1-2
        // - `${name}_acc`                         Flat name for coordinate acceleration, used in SIMM and OpenSim 1-2 when using RRA or CMC (RARE)?
        // - `/${name}/value`                      Used in early OpenSim 4.0 betas? Legacy?
        // - `/${name}/speed`                      Early OpenSim 4.0 betas? Legacy?
        // - `/jointset/joint_name/${name}`        OpenSim 4.x
        // - `/jointset/joint_name/${name}/value`  OpenSim 4.x
        // - `/jointset/joint_name/${name}/speed`  OpenSim 4.x
        //
        // Additionally, these exist but don't count here (e.g. found online or from other sources,
        // such as OpenSim GUI v4.6: `MotionDisplayer.java`):
        //
        // - `${name}_torque`                      Generalized coordinate "torque"
        // - `${name}_force`                       Generalized coordinate "force"
        // - `${FORCE_NAME} == some.dot.separated` Means "state of a muscle/force"
        // - `${name}_sig`                         Flat name for a control signal related to a coordinate, used in OpenSim 3.x (RARE)

        // See: `TableUtilities::findStateLabelIndexInternal`

        // Normalize OpenSim 4.x paths to always be `/before/${name}`
        if (const auto split = osc::rsplit_once(name, '/')) {
            if (split->second == "value" or split->second == "speed") {
                name = split->first;  // `/maybe/before/${name}/value` --> `/maybe/before/${name}`
            }
        }

        // Drop any prefixes from a path hierarchy
        if (const auto split = osc::rsplit_once(name, '/')) {
            name = split->second;  // `/maybe/before/${name}` --> `${name}`
        }

        // Handle legacy "Flat name" suffixes
        constexpr auto legacy_suffixes = std::to_array<std::string_view>({"_u", "_vel", "_acc"});
        for (const auto& suffix : legacy_suffixes) {
            if (name.ends_with(suffix)) {
                name = name.substr(0, name.size() - suffix.size());
            }
        }

        // `name` is now normalized, go search for it in `model`
        for (const auto& coordinate : model.getComponentList<OpenSim::Coordinate>()) {
            if (coordinate.getName() == name and coordinate.getMotionType() == OpenSim::Coordinate::Rotational) {
                return &coordinate;
            }
        }
        return nullptr;
    }
}

class opyn::Model::Impl final {
public:
    explicit Impl(const OpenSim::Model& model) :
        model_{construct_built_model(model)}
    {}

    ModelState initial_state() const
    {
        // Copy the working state out of the model, so that the caller gets
        // an independent state.
        return ModelState{SimTK::State{model_.getWorkingState()}};
    }

    std::vector<std::string> rotational_columns_in(const DataFrame& data_frame) const
    {
        std::vector<std::string> rv;
        for (const auto& series : data_frame) {
            if (find_rotational_coordinate_matching_name(model_, series.name())) {
                rv.emplace_back(series.name());
            }
        }
        return rv;
    }

    void realize(ModelState& state, ModelStateStage stage) const
    {
        switch (stage) {
        case ModelStateStage::time:         model_.realizeTime(state.simbody_state());         break;
        case ModelStateStage::position:     model_.realizePosition(state.simbody_state());     break;
        case ModelStateStage::velocity:     model_.realizeVelocity(state.simbody_state());     break;
        case ModelStateStage::dynamics:     model_.realizeDynamics(state.simbody_state());     break;
        case ModelStateStage::acceleration: model_.realizeAcceleration(state.simbody_state()); break;
        case ModelStateStage::report:       model_.realizeReport(state.simbody_state());       break;  // NOLINT(bugprone-branch-clone)
        default:                            model_.realizeReport(state.simbody_state());       break;
        }
    }

    size_t num_coordinates() const
    {
        const auto lst = model_.getComponentList<OpenSim::Coordinate>();
        return std::distance(lst.begin(), lst.end());
    }

    std::vector<Symbol> coordinates() const
    {
        std::vector<Symbol> rv;
        for (const auto& coordinate : model_.getComponentList<OpenSim::Coordinate>()) {
            rv.emplace_back(coordinate.getAbsolutePathString());
        }
        return rv;
    }

    double get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const
    {
        const auto& coord = model_.getComponent<OpenSim::Coordinate>(std::string{coordinate});
        return coord.getValue(model_state.simbody_state());
    }

    void set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const
    {
        const auto& coord = model_.getComponent<OpenSim::Coordinate>(std::string{coordinate});
        coord.setValue(model_state.simbody_state(), value);
    }

    size_t num_outputs() const
    {
        size_t rv = 0;
        for (const auto& component : model_.getComponentList()) {
            rv += component.getNumOutputs();
        }
        return rv;
    }

    std::vector<Symbol> outputs() const
    {
        std::vector<Symbol> rv;
        rv.reserve(outputs_.size());
        for (const auto& [symbol, output] : outputs_) {
            rv.push_back(symbol);
        }
        return rv;
    }

    OutputValue get_output_value(const ModelState& model_state, const Symbol& output) const
    {
        const auto it = outputs_.find(output);
        if (it == outputs_.end()) {
            std::stringstream ss;
            ss << static_cast<std::string>(output) << "Cannot find this output in the model";
            throw std::runtime_error{std::move(ss).str()};
        }
        return output_extraction_system().read_value(model_state.simbody_state(), *it->second);
    }

    std::vector<osc::SceneDecoration> decorations(
        osc::SceneCache& scene_cache,
        const ModelState& model_state,
        const OpenSimDecorationOptions& open_sim_decoration_options) const
    {
        return GenerateModelDecorations(
            scene_cache,
            model_,
            model_state.simbody_state(),
            open_sim_decoration_options
        );
    }

private:
    OpenSim::Model model_;
    ankerl::unordered_dense::map<Symbol, const OpenSim::AbstractOutput*> outputs_{collect_model_outputs(model_)};
};

opyn::Model::Model(const OpenSim::Model& model) :
    impl_{osc::make_cow<Impl>(model)}
{}

opyn::ModelState opyn::Model::initial_state() const
{
    return impl_->initial_state();
}

std::vector<std::string> opyn::Model::rotational_columns_in(const DataFrame& data_frame) const
{
    return impl_->rotational_columns_in(data_frame);
}

void opyn::Model::realize(ModelState& model_state, ModelStateStage model_state_stage) const
{
    impl_->realize(model_state, model_state_stage);
}

size_t opyn::Model::num_coordinates() const
{
    return impl_->num_coordinates();
}

std::vector<opyn::Symbol> opyn::Model::coordinates() const
{
    return impl_->coordinates();
}

double opyn::Model::get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const
{
    return impl_->get_coordinate_value(model_state, coordinate);
}

void opyn::Model::set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const
{
    impl_->set_coordinate_value(model_state, coordinate, value);
}

size_t opyn::Model::num_outputs() const
{
    return impl_->num_outputs();
}

std::vector<opyn::Symbol> opyn::Model::outputs() const
{
    return impl_->outputs();
}

opyn::OutputValue opyn::Model::get_output_value(const ModelState& model_state, const Symbol& output) const
{
    return impl_->get_output_value(model_state, output);
}

std::vector<osc::SceneDecoration> opyn::Model::decorations(
    osc::SceneCache& scene_cache,
    const ModelState& model_state,
    const OpenSimDecorationOptions& open_sim_decoration_options) const
{
    return impl_->decorations(scene_cache, model_state, open_sim_decoration_options);
}
