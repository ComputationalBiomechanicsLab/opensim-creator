#pragma once

#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>

#include <liboscar/utilities/c_string_view.h>

#include <optional>
#include <string>
#include <utility>

namespace opyn
{
    // A message produced by a `ScalingStep`'s validation check.
    class ScalingStepValidationMessage {
    public:

        // Constructs a validation message that's related to the value(s) held in
        // a property with name `propertyName` on the `ScalingStep`.
        explicit ScalingStepValidationMessage(
            std::string propertyName,
            ScalingStepValidationState state,
            std::string message) :

            maybe_property_name_{std::move(propertyName)},
            state_{state},
            message_{std::move(message)}
        {}

        // Constructs a validation message that's in some (general) way related to
        // the `ScalingStep` that produced it.
        explicit ScalingStepValidationMessage(
            ScalingStepValidationState state,
            std::string message) :

            state_{state},
            message_{std::move(message)}
        {}

        std::optional<osc::CStringView> try_get_property_name() const
        {
            return not maybe_property_name_.empty() ? std::optional{maybe_property_name_} : std::nullopt;
        }
        ScalingStepValidationState get_state() const { return state_; }
        osc::CStringView get_message() const { return message_; }

    private:
        std::string maybe_property_name_;
        ScalingStepValidationState state_;
        std::string message_;
    };
}
