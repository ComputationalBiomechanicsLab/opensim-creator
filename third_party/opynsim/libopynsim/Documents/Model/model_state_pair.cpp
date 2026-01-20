#include "model_state_pair.h"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

const OpenSim::Component& osc::ModelStatePair::implGetComponent() const { return implGetModel(); }
OpenSim::Component& osc::ModelStatePair::implUpdComponent() { return implUpdModel(); }
