#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <liboscar/Utils/UID.h>

namespace osc { class IModelStatePair; }

namespace osc
{
    // a cheap-to-copy holder for top-level model+state info
    //
    // handy for caches that need to check if the info has changed
    class ModelStatePairInfo final {
    public:
        ModelStatePairInfo();
        explicit ModelStatePairInfo(const IModelStatePair&);

        float getFixupScaleFactor() const { return m_FixupScaleFactor; }

        friend bool operator==(const ModelStatePairInfo&, const ModelStatePairInfo&) = default;

    private:
        UID m_ModelVersion;
        UID m_StateVersion;
        OpenSim::ComponentPath m_Selection;
        OpenSim::ComponentPath m_Hover;
        float m_FixupScaleFactor = 1.0f;
    };
}
