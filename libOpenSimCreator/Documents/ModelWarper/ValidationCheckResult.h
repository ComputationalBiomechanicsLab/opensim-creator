#pragma once

#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>

#include <liboscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc::mow
{
    // the result of a runtime validation check against part of a model warping input
    class ValidationCheckResult final {
    public:
        ValidationCheckResult(
            std::string description_,
            bool passOrFail_) :

            m_Description{std::move(description_)},
            m_State{passOrFail_ ? ValidationCheckState::Ok : ValidationCheckState::Error}
        {}

        ValidationCheckResult(
            std::string description_,
            ValidationCheckState state_) :

            m_Description{std::move(description_)},
            m_State{state_}
        {}

        friend bool operator==(const ValidationCheckResult&, const ValidationCheckResult&) = default;

        CStringView description() const { return m_Description; }
        ValidationCheckState state() const { return m_State; }
        bool is_error() const { return m_State == ValidationCheckState::Error; }

    private:
        std::string m_Description;
        ValidationCheckState m_State;
    };
}
