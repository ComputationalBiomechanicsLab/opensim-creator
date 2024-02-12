#pragma once

#include <filesystem>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class FrameDefinitionLookup final {
    public:
        FrameDefinitionLookup() = default;
        FrameDefinitionLookup(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );
    };
}
