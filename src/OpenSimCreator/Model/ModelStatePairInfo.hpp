#pragma once

#include <oscar/Utils/UID.hpp>

#include <OpenSim/Common/ComponentPath.h>

namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    // a cheap-to-copy holder for top-level model+state info
    //
    // handy for caches that need to check if the info has changed
    class ModelStatePairInfo final {
    public:
        ModelStatePairInfo();
        explicit ModelStatePairInfo(VirtualConstModelStatePair const&);

        float getFixupScaleFactor() const { return m_FixupScaleFactor; }
    private:
        friend bool operator==(ModelStatePairInfo const&, ModelStatePairInfo const&) noexcept;

        UID m_ModelVersion;
        UID m_StateVersion;
        OpenSim::ComponentPath m_Selection;
        OpenSim::ComponentPath m_Hover;
        float m_FixupScaleFactor = 1.0f;
    };

    bool operator==(ModelStatePairInfo const&, ModelStatePairInfo const&) noexcept;
    bool operator!=(ModelStatePairInfo const&, ModelStatePairInfo const&) noexcept;
}