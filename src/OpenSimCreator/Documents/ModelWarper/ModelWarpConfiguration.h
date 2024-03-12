#pragma once

#include <oscar/Maths/CommonFunctions.h>

#include <filesystem>

namespace OpenSim { class Model; }

namespace osc::mow
{
    // top-level runtime configuration for warping a single OpenSim model
    class ModelWarpConfiguration final {
    public:
        ModelWarpConfiguration() = default;

        // load from the associated model warp configuration file that sits relative to the osim file
        ModelWarpConfiguration(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&
        );

        float getWarpBlendingFactor() const { return m_WarpBlendingFactor; }
        void setWarpBlendingFactor(float v) { m_WarpBlendingFactor = saturate(v); }

        bool getShouldDefaultMissingFrameWarpsToIdentity() const { return m_ShouldDefaultMissingFrameWarpsToIdentity; }
        void setShouldDefaultMissingFrameWarpsToIdentity(bool v) { m_ShouldDefaultMissingFrameWarpsToIdentity = v; }

    private:
        float m_WarpBlendingFactor = 1.0f;
        bool m_ShouldDefaultMissingFrameWarpsToIdentity = false;
    };
}
