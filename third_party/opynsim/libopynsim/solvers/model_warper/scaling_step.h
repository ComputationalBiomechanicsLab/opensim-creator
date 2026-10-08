#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameter_declaration.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>

#include <OpenSim/Common/Component.h>
#include <liboscar/utilities/c_string_view.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    // An abstract base class for a single model-scaling step.
    //
    // Scaling steps are applied one-by-one to a copy of the source model in
    // order to yield the "result" or "scaled" model. Each scaling step can
    // request external data (`ScalingParameterDeclaration`).
    class ScalingStep : public OpenSim::Component {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ScalingStep, Component)

    public:
        OpenSim_DECLARE_PROPERTY(enabled, bool, "toggles applying this scaling step when scaling the model");
        OpenSim_DECLARE_PROPERTY(label, std::string, "a user-facing label for the scaling step");

    protected:
        explicit ScalingStep(std::string_view label)
        {
            constructProperty_enabled(true);
            constructProperty_label(std::string{label});
        }

    public:
        // Returns a user-facing label that describes this `ScalingStep`.
        osc::CStringView label() const { return get_label(); }

        // Sets this `ScalingStep`'s user-facing label.
        void set_label(osc::CStringView newLabel) { set_label(std::string{newLabel}); }

        // Calls `callback` with each parameter declaration that this `ScalingStep` accepts
        // at scaling-time.
        //
        // It is expected that higher-level engines provide values that satisfy these
        // declarations to `apply_scaling_step`.
        void for_each_scaling_parameter_declaration(
            const std::function<void(const ScalingParameterDeclaration&)>& callback) const
        {
            impl_for_each_scaling_parameter_declaration(callback);
        }

        // Returns a sequence of `ScalingStepValidationMessage`, which should be empty,
        // or non-errors, before higher-level engines call `apply_scaling_step` (otherwise,
        // an exception may be thrown by `apply_scaling_step`).
        std::vector<ScalingStepValidationMessage> validate(
            ScalingCache& scalingCache,
            const ScalingParameters& scalingParameters,
            const OpenSim::Model& sourceModel) const
        {
            return impl_validate(scalingCache, scalingParameters, sourceModel);
        }

        // Applies this `ScalingStep`'s scaling function in-place to the `resultModel`. The
        // original `sourceModel` is also provided, if relevant.
        //
        // It is expected that `scalingParameters` contains at least the scaling parameter
        // values that match the declarations emitted by `for_each_scaling_parameter_declaration`.
        void apply_scaling_step(
            ScalingCache& scalingCache,
            const ScalingParameters& scalingParameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const
        {
            if (get_enabled()) {
                impl_apply_scaling_step(scalingCache, scalingParameters, sourceModel, resultModel);
            }
        }
    private:
        // Implementors should provide the callback with any `ScalingParameterDeclaration`s in order
        // to ensure that the runtime can later provide the `ScalingParameterValue` during model
        // scaling.
        virtual void impl_for_each_scaling_parameter_declaration(const std::function<void(const ScalingParameterDeclaration&)>&) const
        {}

        // Implementors should return any validation warnings/errors related to this scaling step
        // (e.g. incorrect property value, missing external data, etc.).
        virtual std::vector<ScalingStepValidationMessage> impl_validate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&) const
        {
            return {};  // i.e. by default, return no validation errors.
        }

        // Implementors should apply their scaling to the result model (the source model is also
        // available). Any computationally expensive scaling steps should be performed via
        // the `ScalingCache`.
        virtual void impl_apply_scaling_step(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model&) const
        {}
    };
}
