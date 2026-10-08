#include "model_state_pair.h"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

const OpenSim::Component& opyn::ModelStatePair::impl_get_component() const { return impl_get_model(); }
OpenSim::Component& opyn::ModelStatePair::impl_upd_component() { return impl_upd_model(); }
