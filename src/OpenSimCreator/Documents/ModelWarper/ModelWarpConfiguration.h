#pragma once

#include <filesystem>

namespace OpenSim { class Model; }

namespace osc::mow
{
    class ModelWarpConfiguration final {
    public:
        ModelWarpConfiguration() = default;
        ModelWarpConfiguration(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&
        );
    };
}
