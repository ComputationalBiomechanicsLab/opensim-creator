#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <liboscar/utilities/uid.h>

namespace opyn { class ModelStatePair; }

namespace opyn
{
    // a cheap-to-copy holder for top-level model+state info
    //
    // handy for caches that need to check if the info has changed
    class ModelStatePairInfo final {
    public:
        ModelStatePairInfo();
        explicit ModelStatePairInfo(const opyn::ModelStatePair&);

        float get_fixup_scale_factor() const { return fixup_scale_factor_; }

        friend bool operator==(const ModelStatePairInfo&, const ModelStatePairInfo&) = default;

    private:
        osc::UID model_version_;
        osc::UID state_version_;
        OpenSim::ComponentPath selection_;
        OpenSim::ComponentPath hover_;
        float fixup_scale_factor_ = 1.0f;
    };
}
