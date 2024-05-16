#include "AppMetadata.h"

#include <oscar/Utils/CStringView.h>

#include <optional>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

std::string osc::calc_full_application_name_with_version_and_build_id(AppMetadata const& metadata)
{
    std::stringstream ss;
    ss << calc_human_readable_application_name(metadata);
    if (auto version = metadata.maybe_version_string()) {
        ss << " v" << *version;
    }
    if (auto buildID = metadata.maybe_build_id()) {
        ss << " (build " << *buildID << ')';
    }
    return std::move(ss).str();
}

CStringView osc::calc_human_readable_application_name(AppMetadata const& metadata)
{
    return metadata.maybe_long_application_name().value_or(metadata.application_name());
}
