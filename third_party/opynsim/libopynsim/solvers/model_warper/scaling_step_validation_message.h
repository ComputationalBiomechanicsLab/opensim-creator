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

            m_MaybePropertyName{std::move(propertyName)},
            m_State{state},
            m_Message{std::move(message)}
        {}

        // Constructs a validation message that's in some (general) way related to
        // the `ScalingStep` that produced it.
        explicit ScalingStepValidationMessage(
            ScalingStepValidationState state,
            std::string message) :

            m_State{state},
            m_Message{std::move(message)}
        {}

        std::optional<osc::CStringView> tryGetPropertyName() const
        {
            return not m_MaybePropertyName.empty() ? std::optional{m_MaybePropertyName} : std::nullopt;
        }
        ScalingStepValidationState getState() const { return m_State; }
        osc::CStringView getMessage() const { return m_Message; }

    private:
        std::string m_MaybePropertyName;
        ScalingStepValidationState m_State;
        std::string m_Message;
    };
}
