#pragma once

#include <filesystem>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class FrameWarpLookup final {
    public:
        FrameWarpLookup() = default;
        FrameWarpLookup(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );
    };
}
