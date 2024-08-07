#include "IValidateable.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <oscar/Utils/Algorithms.h>

using namespace osc::mow;

std::vector<ValidationCheckResult> osc::mow::IValidateable::implValidate(const WarpableModel&) const
{
    return {};
}

ValidationCheckState osc::mow::IValidateable::implState(const WarpableModel& root) const
{
    ValidationCheckState worst = ValidationCheckState::Ok;
    for (const auto& c : validate(root)) {
        worst = max(worst, c.state());
        if (worst == ValidationCheckState::Error) {
            break;
        }
    }
    return worst;
}
