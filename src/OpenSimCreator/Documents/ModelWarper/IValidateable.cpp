#include "IValidateable.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <oscar/Utils/Algorithms.h>

using namespace osc::mow;

ValidationCheckState osc::mow::IValidateable::implState() const
{
    ValidationCheckState worst = ValidationCheckState::Ok;
    for (auto const& c : validate()) {
        worst = max(worst, c.state());
        if (worst == ValidationCheckState::Error) {
            break;
        }
    }
    return worst;
}
