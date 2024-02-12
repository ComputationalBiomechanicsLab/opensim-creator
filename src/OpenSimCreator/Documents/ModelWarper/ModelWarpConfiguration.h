#pragma once

#include <filesystem>

namespace osc::mow
{
    class ModelWarpConfiguration final {
    public:
        ModelWarpConfiguration() = default;
        explicit ModelWarpConfiguration(std::filesystem::path const& osimPath);
    };
}
