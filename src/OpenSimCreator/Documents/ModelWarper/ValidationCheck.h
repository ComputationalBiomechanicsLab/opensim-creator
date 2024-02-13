#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>

#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc::mow
{
    class ValidationCheck final {
    public:
        ValidationCheck(
            std::string description_,
            bool passOrFail_) :

            m_Description{std::move(description_)},
            m_State{passOrFail_ ? ValidationState::Ok : ValidationState::Error}
        {}

        ValidationCheck(
            std::string description_,
            ValidationState state_) :

            m_Description{std::move(description_)},
            m_State{state_}
        {}

        CStringView description() const { return m_Description; }
        ValidationState state() const { return m_State; }

    private:
        std::string m_Description;
        ValidationState m_State;
    };
}
