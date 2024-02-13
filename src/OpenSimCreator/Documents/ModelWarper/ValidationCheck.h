#pragma once

#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc::mow
{
    class ValidationCheck final {
    public:
        enum class State { Ok, Warning, Error };

        ValidationCheck(
            std::string description_,
            bool passOrFail_) :

            m_Description{std::move(description_)},
            m_State{passOrFail_ ? State::Ok : State::Error}
        {}

        ValidationCheck(
            std::string description_,
            State state_) :

            m_Description{std::move(description_)},
            m_State{state_}
        {}

        CStringView description() const { return m_Description; }
        State state() const { return m_State; }

    private:
        std::string m_Description;
        State m_State;
    };
}
