#include "IValidateable.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>

#include <oscar/Utils/Algorithms.h>

using namespace osc::mow;

ValidationState osc::mow::IValidateable::implState() const
{
    ValidationState worst = ValidationState::Ok;
    for (auto const& c : validate()) {
        worst = max(worst, c.state());
        if (worst == ValidationState::Error) {
            break;
        }
    }
    return worst;
}
