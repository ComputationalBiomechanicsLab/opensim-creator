#include "AppSettings.hpp"

#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <toml++/toml.h>
#include <ankerl/unordered_dense.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using osc::AppSettings;
using osc::AppSettingValue;
using osc::AppSettingValueType;
using osc::CStringView;
using osc::SynchronizedValue;

namespace
{
    constexpr CStringView c_ConfigFileHeader =
R"(# configuration options
#
# you can manually add config options here: they will override the system configuration file, e.g.:
#
#     initial_tab = "LearnOpenGL/Blending"
#
# beware: this file is overwritten by the application when it detects that you have made changes

)";

    // the "scope" of an application setting value
    enum class AppSettingScope {

        // set by a user-level configuration file, or by runtime code
        // (e.g. the user clicked a checkbox or similar)
        //
        // user settings override system-wide settings
        User = 0,

        // set by a readonly system-level configuration file
        System,

        NUM_OPTIONS,
    };

    // a value stored in the AppSettings lookup table
    class AppSettingsLookupValue final {
    public:
        AppSettingsLookupValue(AppSettingScope scope, AppSettingValue value) :
            m_Scope{scope},
            m_Value{std::move(value)}
        {
        }

        AppSettingValue const& getValue() const
        {
            return m_Value;
        }

        AppSettingScope getScope() const
        {
            return m_Scope;
        }

    private:
        AppSettingScope m_Scope;
        AppSettingValue m_Value;
    };

    // a lookup containing all app setting values
    class AppSettingsLookup final {
    public:
        std::optional<AppSettingValue> getValue(std::string_view key) const
        {
            if (auto const v = lookup(key))
            {
                return v->getValue();
            }
            else
            {
                return std::nullopt;
            }
        }

        void setValue(
            std::string_view key,
            AppSettingScope scope,
            AppSettingValue value)
        {
            m_Data.insert_or_assign(
                key,
                AppSettingsLookupValue{scope, std::move(value)}
            );
        }

        std::optional<AppSettingScope> getScope(std::string_view key) const
        {
            if (auto const v = lookup(key))
            {
                return v->getScope();
            }
            else
            {
                return std::nullopt;
            }
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
        AppSettingsLookupValue const* lookup(std::string_view key) const
        {
            if (auto const it = m_Data.find(key); it != m_Data.cend())
            {
                return &it->second;
            }
            else
            {
                return nullptr;
            }
        }

        // see: ankerl/unordered_dense documentation for heterogeneous lookups
        struct transparent_string_hash final {
            using is_transparent = void;
            using is_avalanching = void;

            [[nodiscard]] auto operator()(std::string_view str) const -> uint64_t {
                return ankerl::unordered_dense::hash<std::string_view>{}(str);
            }
        };

        using Storage = ankerl::unordered_dense::map<
            std::string,
            AppSettingsLookupValue,
            transparent_string_hash,
            std::equal_to<>
        >;
        Storage m_Data;
    };

    // if available, returns the path to the system-wide configuration file
    std::optional<std::filesystem::path>  TryGetSystemConfigPath(
        std::string_view applicationConfigFileName_)
    {
        // copied from the legacy `osc::AppConfig` implementation for backwards
        // compatability with existing config files

        std::filesystem::path p = osc::CurrentExeDir();
        bool exists = false;

        while (p.has_filename())
        {
            std::filesystem::path maybeConfig = p / applicationConfigFileName_;
            if (std::filesystem::exists(maybeConfig))
            {
                p = maybeConfig;
                exists = true;
                break;
            }

            // HACK: there is a file at "MacOS/$configName", which is where the config
            // is relative to SDL_GetBasePath. current_exe_dir should be fixed
            // accordingly.
            std::filesystem::path maybeMacOSConfig = p / "MacOS" / applicationConfigFileName_;
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

    // it available, returns the path to the user-level configuration file
    //
    // creates a "blank" user-level configuration file if one doesn't already exist
    std::optional<std::filesystem::path> TryGetUserConfigPath(
        std::string_view organizationName_,
        std::string_view applicationName_,
        std::string_view applicationConfigFileName_)
    {
        auto userDir = osc::GetUserDataDir(std::string{organizationName_}, std::string{applicationName_});
        auto fullPath = userDir / applicationConfigFileName_;

        if (std::filesystem::exists(fullPath))
        {
            return fullPath;
        }
        else if (auto userFileStream = std::ofstream{fullPath})  // create blank user file
        {
            userFileStream << c_ConfigFileHeader;
            return fullPath;
        }
        else
        {
            return std::nullopt;
        }
    }

    toml::table ParseTomlFileOrWarn(std::filesystem::path const& p)
    {
        try
        {
            return toml::parse_file(p.string());
        }
        catch (std::exception const& ex)
        {
            osc::log::warn("error parsing %s: %s", p.string().c_str(), ex.what());
            osc::log::warn("the application will skip loading this configuration file, but you might want to fix it");
            return toml::table{};
        }
    }

    // loads application settings, located at `configPath` into the given lookup (`out`)
    // at the given `scope` level
    void LoadAppSettingsFromDiskIntoLookup(
        std::filesystem::path const& configPath,
        AppSettingScope scope,
        AppSettingsLookup& out)
    {
        toml::table const config = ParseTomlFileOrWarn(configPath);

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
        stack.emplace_back("", config);  // required for .begin()+1
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
                    ++cur.iterator;
                    break;
                }
                else if (auto const* stringValue = node.as_string())
                {
                    out.setValue(keyPrefix + std::string{k.str()}, scope, AppSettingValue{**stringValue});
                }
                else if (auto const* boolValue = node.as_boolean())
                {
                    out.setValue(keyPrefix + std::string{k.str()}, scope, AppSettingValue{**boolValue});
                }
            }

            if (!recursing)
            {
                stack.pop_back();
            }
        }
    }

    // loads an app settings lookup from the given paths
    AppSettingsLookup LoadAppSettingsLookupFromDisk(
        std::optional<std::filesystem::path> const& maybeSystemConfigPath,
        std::optional<std::filesystem::path> const& maybeUserConfigPath)
    {
        AppSettingsLookup rv;
        if (maybeSystemConfigPath)
        {
            LoadAppSettingsFromDiskIntoLookup(*maybeSystemConfigPath, AppSettingScope::System, rv);
        }
        if (maybeUserConfigPath)
        {
            LoadAppSettingsFromDiskIntoLookup(*maybeUserConfigPath, AppSettingScope::User, rv);
        }
        return rv;
    }

    // returns (tableName, valueName) parts of the given settings key
    std::pair<std::string_view, std::string_view> SplitAtLastElement(std::string_view k)
    {
        if (auto const pos = k.rfind('/'); pos != std::string_view::npos)
        {
            return std::pair{k.substr(0, pos), k.substr(pos+1)};
        }
        else
        {
            return std::pair{std::string_view{}, k};
        }
    }

    toml::table& GetDeepestTable(
        toml::table& root,
        std::unordered_map<std::string, toml::table*>& lut,
        std::string_view tablePath)
    {
        // edge-case
        if (tablePath.empty())
        {
            return root;
        }

        // iterate through each part of the given path (e.g. a/b/c)
        size_t const depth = std::count(tablePath.begin(), tablePath.end(), '/') + 1;
        toml::table* currentTable = &root;
        std::string_view currentPath = tablePath.substr(0, tablePath.find('/'));

        for (size_t i = 0; i < depth; ++i)
        {
            std::string_view const name = SplitAtLastElement(currentPath).second;

            // insert into LUT that's used by this code
            auto [it, inserted] = lut.try_emplace(std::string{currentPath}, nullptr);

            // if necessary, insert into TOML document (or re-use existing node)
            toml::node* n = &currentTable->insert(name, toml::table{}).first->second;

            if (auto* t = dynamic_cast<toml::table*>(n))
            {
                it->second = t;
                currentTable = it->second;
            }
            else
            {
                // edge-case: it already exists in the TOML document as a non-table node,
                //            which can happen if (e.g.) a user defines setting values with
                //            both 'a/b' and 'a/b/c'
                //
                //            in this case, overwrite the value with a LUT (it's a user/app error)
                it->second = dynamic_cast<toml::table*>(&currentTable->insert_or_assign(name, toml::table{}).first->second);
                currentTable =  it->second;
            }

            currentPath = tablePath.substr(0, tablePath.find('/', currentPath.size()+1));
        }
        return *currentTable;
    }

    void InsertIntoTomlTable(
        toml::table& table,
        std::string_view key,
        AppSettingValue const& value)
    {
        static_assert(osc::NumOptions<AppSettingValueType>() == 3);

        switch (value.type())
        {
        case AppSettingValueType::Bool:
            table.insert(key, value.toBool());
            break;
        case AppSettingValueType::String:
        case AppSettingValueType::Color:
        default:
            table.insert(key, value.toString());
            break;
        }
    }

    toml::table ToTomlTable(AppSettingsLookup const& vs)
    {
        toml::table rv;
        std::unordered_map<std::string, toml::table*> nodeLUT;
        for (auto const& [k, v] : vs)
        {
            if (v.getScope() != AppSettingScope::User)
            {
                continue;  // skip non-user setting values
            }

            auto [tableName, valueName] = SplitAtLastElement(k);
            toml::table& t = GetDeepestTable(rv, nodeLUT, tableName);
            InsertIntoTomlTable(t, valueName, v.getValue());
        }

        return rv;
    }

    // thread-unsafe data storage for application settings
    //
    // a higher level of the system must make sure this is mutex-guarded
    class ThreadUnsafeAppSettings final {
    public:
        ThreadUnsafeAppSettings(
            std::string_view organizationName_,
            std::string_view applicationName_,
            std::string_view applicationConfigFileName_) :

            m_SystemConfigPath{TryGetSystemConfigPath(applicationConfigFileName_)},
            m_UserConfigPath{TryGetUserConfigPath(organizationName_, applicationName_, applicationConfigFileName_)}
        {
        }

        std::optional<std::filesystem::path> getSystemConfigurationFileLocation() const
        {
            return m_SystemConfigPath;
        }

        std::optional<AppSettingValue> getValue(std::string_view key) const
        {
            return m_AppSettings.getValue(key);
        }

        void setValue(std::string_view key, AppSettingValue value)
        {
            m_AppSettings.setValue(key, AppSettingScope::User, std::move(value));
            m_IsDirty = true;
        }

        std::optional<std::filesystem::path> getValueFilesystemSource(
            std::string_view key) const
        {
            std::optional<AppSettingScope> const scope = m_AppSettings.getScope(key);
            if (!scope)
            {
                return std::nullopt;
            }

            static_assert(osc::NumOptions<AppSettingScope>() == 2);
            switch (*scope)
            {
            case AppSettingScope::System:
                return m_SystemConfigPath;
            case AppSettingScope::User:
            default:
                return m_UserConfigPath;
            }
        }

        void sync()
        {
            if (!m_IsDirty)
            {
                // no changes need to be synchronized
                return;
            }

            if (!m_UserConfigPath)
            {
                if (!std::exchange(m_WarningAboutMissingUserConfigIssued, true))
                {
                    osc::log::warn("application settings could not be synchronized: could not find a user configuration file path");
                    osc::log::warn("this can happen if (e.g.) your user data directory has incorrect permissions");
                }
                return;
            }

            std::ofstream outFile{*m_UserConfigPath};
            if (!outFile)
            {
                if (!std::exchange(m_WarningAboutCannotOpenUserConfigFileIssued, true))
                {
                    osc::log::warn("%s: could not open for writing: user settings will not be saved", m_UserConfigPath->string().c_str());
                }
                return;
            }

            toml::table const settingsAsToml = ToTomlTable(m_AppSettings);
            outFile << c_ConfigFileHeader;
            outFile << settingsAsToml;
            outFile << '\n';  // trailing newline (not added by tomlplusplus?)

            m_IsDirty = false;
        }
    private:
        std::optional<std::filesystem::path> m_SystemConfigPath;
        std::optional<std::filesystem::path> m_UserConfigPath;
        AppSettingsLookup m_AppSettings = LoadAppSettingsLookupFromDisk(m_SystemConfigPath, m_UserConfigPath);
        bool m_IsDirty = false;
        bool m_WarningAboutMissingUserConfigIssued = false;
        bool m_WarningAboutCannotOpenUserConfigFileIssued = false;
    };
}

class osc::AppSettings::Impl final {
public:
    Impl(
        std::string_view organizationName_,
        std::string_view applicationName_,
        std::string_view applicationConfigFileName_) :

        m_GuardedData{organizationName_, applicationName_, applicationConfigFileName_}
    {
    }

    std::optional<std::filesystem::path> getSystemConfigurationFileLocation() const
    {
        return m_GuardedData.lock()->getSystemConfigurationFileLocation();
    }

    std::optional<AppSettingValue> getValue(std::string_view key) const
    {
        return m_GuardedData.lock()->getValue(key);
    }

    void setValue(std::string_view key, AppSettingValue value)
    {
        m_GuardedData.lock()->setValue(key, std::move(value));
    }

    std::optional<std::filesystem::path> getValueFilesystemSource(
        std::string_view key) const
    {
        return m_GuardedData.lock()->getValueFilesystemSource(key);
    }

    void sync()
    {
        m_GuardedData.lock()->sync();
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
//
// the idea of sharing `AppSettings` instances process-wide is inspired
// by Qt's `QSettings` design
namespace
{
    class GlobalAppSettingsLookup final {
    public:
        std::shared_ptr<AppSettings::Impl> get(
            std::string_view organizationName_,
            std::string_view applicationName_,
            std::string_view applicationConfigFileName_)
        {
            auto [it, inserted] = m_Data.try_emplace(Key{organizationName_, applicationName_, applicationConfigFileName_});
            if (inserted)
            {
                it->second = std::make_shared<AppSettings::Impl>(organizationName_, applicationName_, applicationConfigFileName_);
            }
            return it->second;
        }
    private:
        using Key = std::tuple<std::string, std::string, std::string>;

        struct KeyHasher final {
            size_t operator()(Key const& k) const
            {
                return osc::HashOf(std::get<0>(k), std::get<1>(k), std::get<2>(k));
            }
        };

        std::unordered_map<Key, std::shared_ptr<AppSettings::Impl>, KeyHasher> m_Data;
    };

    std::shared_ptr<AppSettings::Impl> GetGloballySharedImplSettings(
        std::string_view organizationName_,
        std::string_view applicationName_,
        std::string_view applicationConfigFileName_)
    {
        static SynchronizedValue<GlobalAppSettingsLookup> s_SettingsLookup;
        return s_SettingsLookup.lock()->get(organizationName_, applicationName_, applicationConfigFileName_);
    }
}

osc::AppSettings::AppSettings(
    std::string_view organizationName_,
    std::string_view applicationName_,
    std::string_view applicationConfigFileName_) :

    m_Impl{GetGloballySharedImplSettings(organizationName_, applicationName_, applicationConfigFileName_)}
{
}
osc::AppSettings::AppSettings(AppSettings const&) = default;
osc::AppSettings::AppSettings(AppSettings&&) noexcept = default;
osc::AppSettings& osc::AppSettings::operator=(AppSettings const&) = default;
osc::AppSettings& osc::AppSettings::operator=(AppSettings&&) noexcept = default;
osc::AppSettings::~AppSettings() noexcept
{
    m_Impl->sync();
}

std::optional<std::filesystem::path> osc::AppSettings::getSystemConfigurationFileLocation() const
{
    return m_Impl->getSystemConfigurationFileLocation();
}

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

std::optional<std::filesystem::path> osc::AppSettings::getValueFilesystemSource(
    std::string_view key) const
{
    return m_Impl->getValueFilesystemSource(key);
}

void osc::AppSettings::sync()
{
    m_Impl->sync();
}
