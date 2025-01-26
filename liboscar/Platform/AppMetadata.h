#pragma once

#include <liboscar/Utils/CStringView.h>

#include <optional>
#include <string>

namespace osc
{
    struct AppMetadata final {

        CStringView human_readable_application_name() const
        {
            return long_application_name ? *long_application_name : application_name;
        }

        std::string application_name_with_version_and_buildid() const;

        std::string organization_name = "oscarorg";
        std::string application_name = "osc";
        std::string config_filename = "osc.toml";
        std::optional<std::string> long_application_name;
        std::optional<std::string> version_string;
        std::optional<std::string> build_id;
        std::optional<std::string> repository_url;
        std::optional<std::string> help_url;
    };
}
