#include "ModelWarpConfiguration.h"

#include <oscar/Platform/Log.h>
#include <toml++/toml.hpp>

#include <filesystem>

osc::mow::ModelWarpConfiguration::ModelWarpConfiguration(
    const std::filesystem::path& osimFileLocation,
    const OpenSim::Model&)
{
    std::filesystem::path maybeWarpconfigLocation = osimFileLocation;
    maybeWarpconfigLocation.replace_extension("warpconfig.toml");
    if (std::filesystem::exists(maybeWarpconfigLocation)) {
        auto parsed = toml::parse_file(maybeWarpconfigLocation.string());
        if (auto globals = parsed.get_as<toml::table>("global_settings")) {
            if (auto defaultWarps = globals->get_as<bool>("should_default_missing_frame_warps_to_identity")) {
                m_ShouldDefaultMissingFrameWarpsToIdentity = defaultWarps->value_or(false);
            }
        }
    }
}
