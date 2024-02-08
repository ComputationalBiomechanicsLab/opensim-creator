#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <optional>
#include <string>

namespace osc
{
    class AppMetadata final {
    public:
        AppMetadata() :
            m_OrganizationName{"oscarorg"},
            m_ApplicationName{"oscar"}
        {
        }

        AppMetadata(
            CStringView organizationName_,
            CStringView applicationName_) :

            m_OrganizationName{organizationName_},
            m_ApplicationName{applicationName_}
        {
        }

        AppMetadata(
            CStringView organizationName_,
            CStringView applicationName_,
            CStringView longApplicationName_,
            CStringView versionString_,
            CStringView buildID_,
            CStringView repositoryURL_) :

            m_OrganizationName{organizationName_},
            m_ApplicationName{applicationName_},
            m_LongApplicationName{longApplicationName_},
            m_VersionString{versionString_},
            m_BuildID{buildID_},
            m_RepositoryURL{repositoryURL_}
        {
        }

        CStringView getOrganizationName() const
        {
            return m_OrganizationName;
        }

        CStringView getApplicationName() const
        {
            return m_ApplicationName;
        }

        std::optional<CStringView> tryGetLongApplicationName() const
        {
            return m_LongApplicationName;
        }

        std::optional<CStringView> tryGetVersionString() const
        {
            return m_VersionString;
        }

        std::optional<CStringView> tryGetBuildID() const
        {
            return m_BuildID;
        }

        std::optional<CStringView> tryGetRepositoryURL() const
        {
            return m_RepositoryURL;
        }

    private:
        std::string m_OrganizationName;
        std::string m_ApplicationName;
        std::optional<std::string> m_LongApplicationName;
        std::optional<std::string> m_VersionString;
        std::optional<std::string> m_BuildID;
        std::optional<std::string> m_RepositoryURL;
    };

    std::string CalcFullApplicationNameWithVersionAndBuild(AppMetadata const&);
    CStringView GetBestHumanReadableApplicationName(AppMetadata const&);
}
