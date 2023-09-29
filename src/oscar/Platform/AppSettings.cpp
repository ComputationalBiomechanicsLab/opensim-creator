#include "AppSettings.hpp"

#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <toml++/toml.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    enum class AppSettingScope {
        User = 0,
        System,
        NUM_OPTIONS,
    };
    constexpr size_t ToIndex(AppSettingScope s) noexcept
    {
        return static_cast<size_t>(s);
    }

    class AppSettingsLookupValue final {
    public:
        AppSettingsLookupValue(
            AppSettingScope scope,
            osc::AppSettingValue value)
        {
            m_Data[ToIndex(scope)] = std::move(value);
        }

        osc::AppSettingValue const& get() const
        {
            static_assert(AppSettingScope::User < AppSettingScope::System);

            auto const hasValue = [](std::optional<osc::AppSettingValue> const& o)
            {
                return o.has_value();
            };
            auto const it = std::find_if(m_Data.begin(), m_Data.end(), hasValue);
            OSC_ASSERT(it != m_Data.end() && it->has_value());
            return **it;
        }
    private:
        std::array<
            std::optional<osc::AppSettingValue>,
            osc::NumOptions<AppSettingScope>()
        > m_Data{};
    };

    class AppSettingsLookup final {
    public:
        std::optional<osc::AppSettingValue> getValue(std::string_view key) const
        {
            if (auto const it = m_Data.find(std::string{key}); it != m_Data.end())
            {
                return it->second.get();
            }
            else
            {
                return std::nullopt;
            }
        }

        void setValue(
            std::string_view key,
            AppSettingScope scope,
            osc::AppSettingValue value)
        {
            m_Data.insert_or_assign(
                std::string{key},
                AppSettingsLookupValue{scope, std::move(value)}
            );
        }

        auto begin() const
        {
            return m_Data.begin();
        }

        auto end() const
        {
            return m_Data.end();
        }
    private:
        std::unordered_map<std::string, AppSettingsLookupValue> m_Data;
    };

    std::optional<std::filesystem::path>  TryGetSystemConfigPath()
    {
        // copied from the legacy `osc::AppConfig` implementation for backwards
        // compatability with existing config files

        std::filesystem::path p = osc::CurrentExeDir();
        bool exists = false;

        while (p.has_filename())
        {
            std::filesystem::path maybeConfig = p / "osc.toml";
            if (std::filesystem::exists(maybeConfig))
            {
                p = maybeConfig;
                exists = true;
                break;
            }

            // HACK: there is a file at "MacOS/osc.toml", which is where the config
            // is relative to SDL_GetBasePath. current_exe_dir should be fixed
            // accordingly.
            std::filesystem::path maybeMacOSConfig = p / "MacOS" / "osc.toml";
            if (std::filesystem::exists(maybeMacOSConfig))
            {
                p = maybeMacOSConfig;
                exists = true;
                break;
            }
            p = p.parent_path();
        }

        return exists ? p : std::optional<std::filesystem::path>{};
    }

    std::optional<std::filesystem::path> TryGetUserConfigPath(
        std::string_view organizationName_,
        std::string_view applicationName_)
    {
        auto userDir = osc::GetUserDataDir(std::string{organizationName_}, std::string{applicationName_});
        auto fullPath = userDir / "osc.toml";

        if (std::filesystem::exists(fullPath))
        {
            return fullPath;
        }
        else if (auto userFileStream = std::ofstream{fullPath})  // create blank user file
        {
            userFileStream << "# this is currently blank because OSC hasn't detected any user-enacted configuration changes\n";
            userFileStream << "#\n";
            userFileStream << "# you can manually add config options here, if you want: they will override the system configuration file, e.g.\n";
            userFileStream << "#\n";
            userFileStream << "# initial_tab = \"LearnOpenGL/Blending\"\n";
            return fullPath;
        }
        else
        {
            return std::nullopt;
        }
    }

    void LoadAppSettings(
        std::filesystem::path const& configPath,
        AppSettingScope scope,
        AppSettingsLookup& out)
    {
        toml::table config;
        try
        {
            config = toml::parse_file(configPath.string());
        }
        catch (std::exception const& ex)
        {
            osc::log::warn("error parsing %s: %s", configPath.string().c_str(), ex.what());
            osc::log::warn("OSC will skip loading this configuration file, but you might want to fix it");
        }

        // crawl the table
        //
        // - every section acts as a key prefix of `$section1/$section2/$key`
        struct StackElement final {
            StackElement(
                std::string_view tableName_,
                toml::table const& table_) :

                tableName{tableName_},
                table{table_}
            {
            }

            std::string_view tableName;
            toml::table const& table;
            toml::table::const_iterator iterator = table.cbegin();
        };

        std::vector<StackElement> stack;
        stack.reserve(16);  // guess
        stack.emplace_back("", config);

        while (!stack.empty())
        {
            std::string const keyPrefix = [&stack]()
            {
                std::string rv;
                for (auto it = stack.begin() + 1; it != stack.end(); ++it)
                {
                    rv += it->tableName;
                    rv += '/';
                }
                return rv;
            }();

            auto& cur = stack.back();
            bool recursing = false;
            for (; cur.iterator != cur.table.cend(); ++cur.iterator)
            {
                auto const& [k, node] = *cur.iterator;
                if (auto const* ptr = node.as_table())
                {
                    stack.emplace_back(k, *ptr);
                    recursing = true;
                    continue;
                }
                else if (auto const* str = node.as_string())
                {
                    out.setValue(keyPrefix + std::string{k.str()}, scope, osc::AppSettingValue{**str});
                }
                else if (auto const* b = node.as_boolean())
                {
                    out.setValue(keyPrefix + std::string{k.str()}, scope, osc::AppSettingValue{**b});
                }
            }

            if (!recursing)
            {
                stack.pop_back();
            }
        }
    }

    AppSettingsLookup LoadAppSettingsFromDisk(
        std::optional<std::filesystem::path> const& maybeSystemConfigPath,
        std::optional<std::filesystem::path> const& maybeUserConfigPath)
    {
        AppSettingsLookup rv;
        if (maybeSystemConfigPath)
        {
            LoadAppSettings(*maybeSystemConfigPath, AppSettingScope::System, rv);
        }
        if (maybeUserConfigPath)
        {
            LoadAppSettings(*maybeUserConfigPath, AppSettingScope::User, rv);
        }
        return rv;
    }

    class ThreadUnsafeAppSettings final {
    public:
        ThreadUnsafeAppSettings(
            std::string_view organizationName_,
            std::string_view applicationName_) :
            m_SystemConfigPath{TryGetSystemConfigPath()},
            m_UserConfigPath{TryGetUserConfigPath(organizationName_, applicationName_)}
        {
            for (auto const& [k, v] : m_AppSettings)
            {
                osc::log::error("%s --> %s", k.c_str(), v.get().toString().c_str());
            }
        }

        std::optional<osc::AppSettingValue> getValue(std::string_view key) const
        {
            return m_AppSettings.getValue(key);
        }

        void setValue(std::string_view key, osc::AppSettingValue value)
        {
            m_AppSettings.setValue(key, AppSettingScope::User, std::move(value));
        }
    private:
        std::optional<std::filesystem::path> m_SystemConfigPath;
        std::optional<std::filesystem::path> m_UserConfigPath;
        AppSettingsLookup m_AppSettings = LoadAppSettingsFromDisk(m_SystemConfigPath, m_UserConfigPath);
    };
}

class osc::AppSettings::Impl final {
public:
    Impl(
        std::string_view organizationName_,
        std::string_view applicationName_) :

        m_GuardedData{organizationName_, applicationName_}
    {
    }

    std::optional<AppSettingValue> getValue(std::string_view key) const
    {
        return m_GuardedData.lock()->getValue(key);
    }

    void setValue(std::string_view key, AppSettingValue value)
    {
        m_GuardedData.lock()->setValue(key, std::move(value));
    }

private:
    SynchronizedValue<ThreadUnsafeAppSettings> m_GuardedData;
};

// flyweight the settings implementations based on organization+appname
//
// the reason why is so that when multiple, independent, threads or parts
// of the application create a new `AppSettings` object, they all have a
// consistent view of the latest keys/values without having to poll the
// disk
namespace
{
    class GlobalAppSettingsLookup final {
    public:
        std::shared_ptr<osc::AppSettings::Impl> get(
            std::string_view organizationName_,
            std::string_view applicationName_)
        {
            auto [it, inserted] = m_Data.try_emplace(Key{organizationName_, applicationName_});
            if (inserted)
            {
                it->second = std::make_shared<osc::AppSettings::Impl>(organizationName_, applicationName_);
            }
            return it->second;
        }
    private:
        using Key = std::pair<std::string, std::string>;

        struct KeyHasher final {
            size_t operator()(Key const& k) const
            {
                return osc::HashOf(k.first, k.second);
            }
        };

        std::unordered_map<Key, std::shared_ptr<osc::AppSettings::Impl>, KeyHasher> m_Data;
    };

    std::shared_ptr<osc::AppSettings::Impl> GetGloballySharedImplSettings(
        std::string_view organizationName_,
        std::string_view applicationName_)
    {
        static osc::SynchronizedValue<GlobalAppSettingsLookup> s_SettingsLookup;
        return s_SettingsLookup.lock()->get(organizationName_, applicationName_);
    }
}

osc::AppSettings::AppSettings(
    std::string_view organizationName_,
    std::string_view applicationName_) :

    m_Impl{GetGloballySharedImplSettings(organizationName_, applicationName_)}
{
}
osc::AppSettings::AppSettings(AppSettings const&) = default;
osc::AppSettings::AppSettings(AppSettings&&) noexcept = default;
osc::AppSettings& osc::AppSettings::operator=(AppSettings const&) = default;
osc::AppSettings& osc::AppSettings::operator=(AppSettings&&) noexcept = default;
osc::AppSettings::~AppSettings() noexcept = default;

std::optional<osc::AppSettingValue> osc::AppSettings::getValue(
    std::string_view key) const
{
    return m_Impl->getValue(key);
}

void osc::AppSettings::setValue(
    std::string_view key,
    AppSettingValue value)
{
    m_Impl->setValue(key, std::move(value));
}
