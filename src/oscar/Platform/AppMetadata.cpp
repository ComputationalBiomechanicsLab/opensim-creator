#include "AppMetadata.h"

#include <oscar/Utils/CStringView.h>

#include <optional>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

std::string osc::CalcFullApplicationNameWithVersionAndBuild(AppMetadata const& metadata)
{
    std::stringstream ss;
    ss << GetBestHumanReadableApplicationName(metadata);
    if (auto version = metadata.tryGetVersionString())
    {
        ss << " v" << *version;
    }
    if (auto buildID = metadata.tryGetBuildID())
    {
        ss << " (build " << *buildID << ')';
    }
    return std::move(ss).str();
}

CStringView osc::GetBestHumanReadableApplicationName(AppMetadata const& metadata)
{
    return metadata.tryGetLongApplicationName().value_or(metadata.getApplicationName());
}
