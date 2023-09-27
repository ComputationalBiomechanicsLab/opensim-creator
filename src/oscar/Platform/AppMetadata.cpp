#include "AppMetadata.hpp"

#include <oscar/Utils/CStringView.hpp>

#include <optional>
#include <sstream>
#include <string>
#include <utility>

std::string osc::CalcFullApplicationNameWithVersionAndBuild(AppMetadata const& metadata)
{
    std::stringstream ss;
    ss << metadata.getApplicationName();
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
