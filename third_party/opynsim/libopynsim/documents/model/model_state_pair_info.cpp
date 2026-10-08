#include "model_state_pair_info.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>

using namespace opyn;

opyn::ModelStatePairInfo::ModelStatePairInfo() = default;

opyn::ModelStatePairInfo::ModelStatePairInfo(const ModelStatePair& msp) :
    model_version_{msp.get_model_version()},
    state_version_{msp.get_state_version()},
    selection_{GetAbsolutePathOrEmpty(msp.get_selected())},
    hover_{GetAbsolutePathOrEmpty(msp.get_hovered())},
    fixup_scale_factor_{msp.get_fixup_scale_factor()}
{}
