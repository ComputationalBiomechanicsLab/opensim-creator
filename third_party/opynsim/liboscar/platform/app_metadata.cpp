#include "app_metadata.h"

#include <format>
#include <string>

std::string osc::AppMetadata::application_name_with_version_and_buildid() const
{
    std::string rv;
    rv += human_readable_application_name();
    if (const auto version_str = version_string()) {
        std::format_to(std::back_inserter(rv), " v{}", *version_str);
    }
    if (const auto build_id_str = build_id()) {
        std::format_to(std::back_inserter(rv), " (build {})", *build_id_str);
    }
    return rv;
}
