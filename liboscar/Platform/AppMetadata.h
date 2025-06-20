#pragma once

#include <liboscar/Utils/CStringView.h>

#include <optional>
#include <string>
#include <string_view>

namespace osc
{
    class AppMetadata final {
    public:
        AppMetadata() = default;

        CStringView organization_name() const { return organization_name_; }
        void set_organization_name(std::string_view new_organization_name) { organization_name_ = new_organization_name; }

        CStringView application_name() const { return application_name_; }
        void set_application_name(std::string_view new_application_name) { application_name_ = new_application_name; }

        CStringView config_filename() const { return config_filename_; }
        void set_config_filename(std::string_view new_config_filename)  { config_filename_ = new_config_filename; }

        std::optional<CStringView> long_appliation_name() const { return long_application_name_; }
        void set_long_application_name(std::string_view new_long_application_name) { long_application_name_ = new_long_application_name; }

        std::optional<CStringView> version_string() const { return version_string_; }
        void set_version_string(std::string_view new_version_string) { version_string_ = new_version_string; }

        std::optional<CStringView> build_id() const { return build_id_; }
        void set_build_id(std::string_view new_build_id) { build_id_ = new_build_id; }

        std::optional<CStringView> repository_url() const { return repository_url_; }
        void set_repository_url(std::string_view new_repository_url) { repository_url_ = new_repository_url; }

        std::optional<CStringView> documentation_url() const { return documentation_url_; }
        void set_documentation_url(std::string_view new_documentation_url) { documentation_url_ = new_documentation_url; }

        std::optional<CStringView> help_url() const { return help_url_; }
        void set_help_url(std::string_view new_help_url) { help_url_ = new_help_url; }

        CStringView human_readable_application_name() const
        {
            return long_application_name_ ? *long_application_name_ : application_name_;
        }

        std::string application_name_with_version_and_buildid() const;

    private:
        std::string organization_name_ = "oscarorg";
        std::string application_name_ = "osc";
        std::string config_filename_ = "osc.toml";
        std::optional<std::string> long_application_name_;
        std::optional<std::string> version_string_;
        std::optional<std::string> build_id_;
        std::optional<std::string> repository_url_;
        std::optional<std::string> documentation_url_;
        std::optional<std::string> help_url_;
    };
}
