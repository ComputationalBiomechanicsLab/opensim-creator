#pragma once

#include <oscar/Utils/CStringView.h>

#include <optional>
#include <string>

namespace osc
{
    class AppMetadata final {
    public:
        AppMetadata() = default;

        AppMetadata(
            CStringView organization_name,
            CStringView application_name) :

            organization_name_{organization_name},
            application_name_{application_name}
        {}

        AppMetadata(
            CStringView organization_name,
            CStringView application_name,
            CStringView long_application_name,
            CStringView version_string,
            CStringView build_id,
            CStringView repository_url,
            CStringView help_url) :

            organization_name_{organization_name},
            application_name_{application_name},
            long_application_name_{long_application_name},
            version_string_{version_string},
            build_id_{build_id},
            repository_url_{repository_url},
            help_url_{help_url}
        {}

        CStringView organization_name() const { return organization_name_; }
        CStringView application_name() const { return application_name_; }
        std::optional<CStringView> maybe_long_application_name() const { return long_application_name_; }
        std::optional<CStringView> maybe_version_string() const { return version_string_; }
        std::optional<CStringView> maybe_build_id() const { return build_id_; }
        std::optional<CStringView> maybe_repository_url() const { return repository_url_; }
        std::optional<CStringView> maybe_help_url() const { return help_url_; }

    private:
        std::string organization_name_ = "oscarorg";
        std::string application_name_ = "oscar";
        std::optional<std::string> long_application_name_;
        std::optional<std::string> version_string_;
        std::optional<std::string> build_id_;
        std::optional<std::string> repository_url_;
        std::optional<std::string> help_url_;
    };

    std::string calc_full_application_name_with_version_and_build_id(const AppMetadata&);
    CStringView calc_human_readable_application_name(const AppMetadata&);
}
