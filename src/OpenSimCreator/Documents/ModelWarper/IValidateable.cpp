#include "IValidateable.h"

#include <algorithm>

using namespace osc::mow;

ValidationCheck::State osc::mow::IValidateable::implState() const
{
    ValidationCheck::State worst = ValidationCheck::State::Ok;
    for (auto const& c : validate()) {
        worst = std::max(worst, c.state());
        if (worst == ValidationCheck::State::Error) {
            break;
        }
    }
    return worst;
}
