#include "IModelStatePair.h"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

const OpenSim::Component& osc::IModelStatePair::implGetComponent() const { return implGetModel(); }
OpenSim::Component& osc::IModelStatePair::implUpdComponent() { return implUpdModel(); }
