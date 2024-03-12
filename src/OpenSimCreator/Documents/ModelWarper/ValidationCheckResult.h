#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <oscar/Utils/CStringView.h>

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

        CStringView description() const { return m_Description; }
        ValidationCheckState state() const { return m_State; }

    private:
        std::string m_Description;
        ValidationCheckState m_State;
    };
}
