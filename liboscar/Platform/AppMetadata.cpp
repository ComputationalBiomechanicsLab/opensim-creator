#include "AppMetadata.h"

#include <sstream>
#include <string>
#include <utility>

std::string osc::AppMetadata::application_name_with_version_and_buildid() const
{
    std::stringstream ss;
    ss << human_readable_application_name();
    if (version_string()) {
        ss << " v" << *version_string();
    }
    if (build_id()) {
        ss << " (build " << *build_id() << ')';
    }
    return std::move(ss).str();
}
