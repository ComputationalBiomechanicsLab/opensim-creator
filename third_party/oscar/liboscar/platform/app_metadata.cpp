#include "app_metadata.h"

#include <sstream>
#include <string>
#include <utility>

std::string osc::AppMetadata::application_name_with_version_and_buildid() const
{
    std::stringstream ss;
    ss << human_readable_application_name();
    if (const auto version_str = version_string()) {
        ss << " v" << *version_str;
    }
    if (const auto build_id_str = build_id()) {
        ss << " (build " << *build_id_str << ')';
    }
    return std::move(ss).str();
}
