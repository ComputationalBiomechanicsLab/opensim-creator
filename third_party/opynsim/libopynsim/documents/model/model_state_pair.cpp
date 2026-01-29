#include "model_state_pair.h"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

const OpenSim::Component& opyn::ModelStatePair::implGetComponent() const { return implGetModel(); }
OpenSim::Component& opyn::ModelStatePair::implUpdComponent() { return implUpdModel(); }
