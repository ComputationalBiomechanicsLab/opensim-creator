#pragma once

#include <string>
#include <utility>

namespace osc::mow
{
    struct ValidationCheck final {
        enum class State { Ok, Warning, Error };

        ValidationCheck(
            std::string description_,
            bool passOrFail_) :

            description{std::move(description_)},
            state{passOrFail_ ? State::Ok : State::Error}
        {
        }

        ValidationCheck(
            std::string description_,
            State state_) :

            description{std::move(description_)},
            state{state_}
        {
        }

        std::string description;
        State state;
    };
}
