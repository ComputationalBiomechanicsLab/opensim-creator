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
            const std::filesystem::path& osimFileLocation,
            const OpenSim::Model&
        );

        float getWarpBlendingFactor() const { return m_WarpBlendingFactor; }
        void setWarpBlendingFactor(float v) { m_WarpBlendingFactor = saturate(v); }

        bool getShouldDefaultMissingFrameWarpsToIdentity() const { return m_ShouldDefaultMissingFrameWarpsToIdentity; }
        void setShouldDefaultMissingFrameWarpsToIdentity(bool v) { m_ShouldDefaultMissingFrameWarpsToIdentity = v; }

        bool getShouldWriteWarpedMeshesToDisk() const { return m_ShouldWriteWarpedMeshesToDisk; }
        void setShouldWriteWarpedMeshesToDisk(bool v) { m_ShouldWriteWarpedMeshesToDisk = v; }

        std::filesystem::path getWarpedMeshesOutputDirectory() const { return m_WarpedMeshesOutputDirectory; }

    private:
        float m_WarpBlendingFactor = 1.0f;
        bool m_ShouldDefaultMissingFrameWarpsToIdentity = false;
        bool m_ShouldWriteWarpedMeshesToDisk = false;
        std::filesystem::path m_WarpedMeshesOutputDirectory = "WarpedGeometry";
    };
}
