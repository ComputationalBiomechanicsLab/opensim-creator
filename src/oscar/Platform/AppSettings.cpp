#include "AppSettings.h"

#include <oscar/Platform/AppSettingScope.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Variant/Variant.h>
#include <oscar/Variant/VariantType.h>

#include <ankerl/unordered_dense.h>
#include <toml++/toml.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr CStringView c_config_file_header =
R"(# configuration options
#
# you can manually add config options here: they will override the system configuration file, e.g.:
#
#     initial_tab = "LearnOpenGL/Blending"
#
# beware: this file is overwritten by the application when it detects that you have made changes

)";

    // a value stored in the AppSettings lookup table
    class AppSettingsLookupValue final {
    public:
        AppSettingsLookupValue(AppSettingScope scope, Variant value) :
            scope_{scope},
            value_{std::move(value)}
        {}

        AppSettingScope scope() const { return scope_; }
        const Variant& value() const { return value_; }

    private:
        AppSettingScope scope_;
        Variant value_;
    };

    // a lookup containing all app setting values
    class AppSettingsLookup final {
    public:
        std::optional<Variant> find_value(std::string_view key) const
        {
            if (const auto v = lookup_or_nullptr(hashmap_, key)) {
                return v->value();
            }
            else {
                return std::nullopt;
            }
        }

        void set_value(std::string_view key, AppSettingScope scope, Variant value)
        {
            AppSettingsLookupValue scoped_value{scope, std::move(value)};

            auto [it, inserted] = hashmap_.try_emplace(key, scoped_value);
            static_assert(AppSettingScope::System < AppSettingScope::User);  // (as an example)
            if (not inserted and it->second.scope() <= scope) {
                it->second = scoped_value;
            }
        }

        std::optional<AppSettingScope> scope_of(std::string_view key) const
        {
            if (const auto v = lookup_or_nullptr(hashmap_, key)) {
                return v->scope();
            }
            else {
                return std::nullopt;
            }
        }

        auto begin() const { return hashmap_.begin(); }
        auto end() const { return hashmap_.end(); }
    private:

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
        Storage hashmap_;
    };

    // if available, returns the path to the system-wide configuration file
    std::optional<std::filesystem::path>  try_get_system_config_path(
        std::string_view application_config_file_name)
    {
        // copied from the legacy `AppConfig` implementation for backwards
        // compatibility with existing settings files

        std::filesystem::path p = current_executable_directory();
        bool exists = false;

        while (p.has_filename()) {
            const std::filesystem::path maybe_config = p / application_config_file_name;
            if (std::filesystem::exists(maybe_config)) {
                p = maybe_config;
                exists = true;
                break;
            }

            // HACK: there is a file at "MacOS/$configName", which is where the settings
            // is relative to SDL_GetBasePath. current_exe_dir should be fixed
            // accordingly.
            const std::filesystem::path maybe_macos_config = p / "MacOS" / application_config_file_name;
            if (std::filesystem::exists(maybe_macos_config)) {
                p = maybe_macos_config;
                exists = true;
                break;
            }
            p = p.parent_path();
        }

        return exists ? p : std::optional<std::filesystem::path>{};
    }

    // if available, returns the path to the user-level configuration file
    //
    // creates a "blank" user-level configuration file if one doesn't already exist
    std::optional<std::filesystem::path> try_get_user_config_path(
        std::string_view organization_name,
        std::string_view application_name,
        std::string_view application_config_file_name)
    {
        const auto user_data_dir = user_data_directory(std::string{organization_name}, std::string{application_name});
        const auto full_path = user_data_dir / application_config_file_name;

        if (std::filesystem::exists(full_path)) {
            return full_path;
        }
        else if (auto new_user_file_stream = std::ofstream{full_path}) {  // create blank user file
            new_user_file_stream << c_config_file_header;
            return full_path;
        }
        else {
            return std::nullopt;
        }
    }

    toml::table parse_toml_file_or_log_warning(const std::filesystem::path& path)
    {
        try {
            return toml::parse_file(path.string());
        }
        catch (const std::exception& ex) {
            log_warn("error parsing %s: %s", path.string().c_str(), ex.what());
            log_warn("the application will skip loading this configuration file, but you might want to fix it");
            return toml::table{};
        }
    }

    // loads application settings, located at `config_path` into the given lookup (`out`)
    // at the given `scope` level
    void load_app_settings_from_disk_into_lookup(
        const std::filesystem::path& config_path,
        AppSettingScope scope,
        AppSettingsLookup& out)
    {
        const toml::table config = parse_toml_file_or_log_warning(config_path);

        // data that's stored in a stack during configuration traversal
        struct StackElement final {
            StackElement(
                std::string_view table_name_,
                const toml::table& table_) :

                table_name{table_name_},
                table{table_}
            {}

            std::string_view table_name;
            const toml::table& table;
            toml::table::const_iterator iterator = table.cbegin();
        };

        // crawl the table
        //
        // - every section acts as a key prefix of `$section1/$section2/$key`
        std::vector<StackElement> stack;
        stack.reserve(16);  // guess
        stack.emplace_back("", config);  // required for .begin()+1
        while (not stack.empty()) {

            const std::string key_prefix = [&stack]()
            {
                std::string rv;
                for (auto it = stack.begin() + 1; it != stack.end(); ++it) {
                    rv += it->table_name;
                    rv += '/';
                }
                return rv;
            }();

            auto& cur = stack.back();
            bool recursing = false;
            for (; cur.iterator != cur.table.cend(); ++cur.iterator) {
                const auto& [k, node] = *cur.iterator;

                if (const auto* ptr = node.as_table()) {
                    stack.emplace_back(k, *ptr);
                    recursing = true;
                    ++cur.iterator;
                    break;
                }
                else if (const auto* int_value = node.as_integer()) {
                    out.set_value(key_prefix + std::string{k.str()}, scope, Variant{static_cast<int>(**int_value)});  // TODO: 64-bit int support
                }
                else if (const auto* float_value = node.as_floating_point()) {
                    out.set_value(key_prefix + std::string{k.str()}, scope, Variant{static_cast<float>(**float_value)});  // TODO: 64-bit float support
                }
                else if (const auto* string_value = node.as_string()) {
                    out.set_value(key_prefix + std::string{k.str()}, scope, Variant{**string_value});
                }
                else if (auto const* bool_value = node.as_boolean()) {
                    out.set_value(key_prefix + std::string{k.str()}, scope, Variant{**bool_value});
                }
            }

            if (not recursing) {
                stack.pop_back();
            }
        }
    }

    // loads an app settings lookup from the given paths
    AppSettingsLookup load_app_settings_lookup_from_disk(
        const std::optional<std::filesystem::path>& maybe_system_config_path,
        const std::optional<std::filesystem::path>& maybe_user_config_path)
    {
        AppSettingsLookup rv;
        if (maybe_system_config_path) {
            load_app_settings_from_disk_into_lookup(*maybe_system_config_path, AppSettingScope::System, rv);
        }
        if (maybe_user_config_path) {
            load_app_settings_from_disk_into_lookup(*maybe_user_config_path, AppSettingScope::User, rv);
        }
        return rv;
    }

    // returns (table_name, value_name) parts of the given settings key
    std::pair<std::string_view, std::string_view> split_at_last_element(std::string_view key)
    {
        if (auto const pos = key.rfind('/'); pos != std::string_view::npos) {
            return std::pair{key.substr(0, pos), key.substr(pos+1)};
        }
        else {
            return std::pair{std::string_view{}, key};
        }
    }

    toml::table& get_deepest_table(
        toml::table& root,
        std::unordered_map<std::string, toml::table*>& lut,
        std::string_view table_path)
    {
        // edge-case
        if (table_path.empty()) {
            return root;
        }

        // iterate through each part of the given path (e.g. a/b/c)
        const size_t depth = rgs::count(table_path, '/') + 1;
        toml::table* current_table = &root;
        std::string_view current_path = table_path.substr(0, table_path.find('/'));

        for (size_t i = 0; i < depth; ++i) {
            const std::string_view name = split_at_last_element(current_path).second;

            // insert into LUT that's used by this code
            auto [it, inserted] = lut.try_emplace(std::string{current_path}, nullptr);

            // if necessary, insert into TOML document (or re-use existing node)
            toml::node* n = &current_table->insert(name, toml::table{}).first->second;

            if (auto* t = dynamic_cast<toml::table*>(n)) {
                it->second = t;
                current_table = it->second;
            }
            else {
                // edge-case: it already exists in the TOML document as a non-table node,
                //            which can happen if (e.g.) a user defines setting values with
                //            both 'a/b' and 'a/b/c'
                //
                //            in this case, overwrite the value with a LUT (it's a user/app error)
                it->second = dynamic_cast<toml::table*>(&current_table->insert_or_assign(name, toml::table{}).first->second);
                current_table =  it->second;
            }

            current_path = table_path.substr(0, table_path.find('/', current_path.size()+1));
        }
        return *current_table;
    }

    void insert_into_toml_table(
        toml::table& table,
        std::string_view key,
        const Variant& value)
    {
        static_assert(num_options<VariantType>() == 9);  // TODO: support more of them

        switch (value.type()) {
        case VariantType::None:
        case VariantType::Bool:
            table.insert(key, value.to<bool>());
            break;
        case VariantType::Float:
            table.insert(key, value.to<float>());
            break;
        case VariantType::Int:
            table.insert(key, value.to<int>());
            break;
        case VariantType::Color:
        case VariantType::String:
        case VariantType::StringName:
        default:
            table.insert(key, value.to<std::string>());
            break;
        }
    }

    toml::table to_toml_table(const AppSettingsLookup& app_settings_lookup)
    {
        toml::table rv;
        std::unordered_map<std::string, toml::table*> node_lookup;
        for (const auto& [key, value] : app_settings_lookup) {

            static_assert(num_options<AppSettingScope>() == 2);
            if (value.scope() != AppSettingScope::User) {
                continue;  // skip non-user-enacted values
            }

            const auto [table_name, value_name] = split_at_last_element(key);
            toml::table& t = get_deepest_table(rv, node_lookup, table_name);
            insert_into_toml_table(t, value_name, value.value());
        }

        return rv;
    }

    // thread-unsafe data storage for application settings
    //
    // a higher level of the system must ensure that this is mutex-guarded
    class ThreadUnsafeAppSettings final {
    public:
        ThreadUnsafeAppSettings(
            std::string_view organization_name,
            std::string_view application_name,
            std::string_view application_config_file_name) :

            system_config_path_{try_get_system_config_path(application_config_file_name)},
            user_config_path_{try_get_user_config_path(organization_name, application_name, application_config_file_name)}
        {}

        std::optional<std::filesystem::path> system_configuration_file_location() const
        {
            return system_config_path_;
        }

        std::optional<Variant> find_value(std::string_view key) const
        {
            return app_settings_lookup_.find_value(key);
        }

        void set_value(std::string_view key, Variant value, AppSettingScope scope)
        {
            app_settings_lookup_.set_value(key, scope, std::move(value));
            is_dirty_ = true;
        }

        void set_value_if_not_found(std::string_view key, Variant value, AppSettingScope scope)
        {
            if (not find_value(key)) {
                set_value(key, value, scope);
            }
        }

        std::optional<std::filesystem::path> find_value_filesystem_source(
            std::string_view key) const
        {
            const std::optional<AppSettingScope> scope = app_settings_lookup_.scope_of(key);
            if (not scope) {
                return std::nullopt;
            }

            static_assert(num_options<AppSettingScope>() == 2);
            switch (*scope) {
            case AppSettingScope::System: return system_config_path_;
            case AppSettingScope::User:   return user_config_path_;
            default:                      return user_config_path_;
            }
        }

        void sync()
        {
            if (not is_dirty_) {
                // no changes need to be synchronized
                return;
            }

            if (not user_config_path_) {
                if (not std::exchange(warning_already_issued_missing_user_config_, true)) {
                    log_warn("application settings could not be synchronized: could not find a user configuration file path");
                    log_warn("this can happen if (e.g.) your user data directory has incorrect permissions");
                }
                return;
            }

            std::ofstream config_stream{*user_config_path_};
            if (not config_stream) {
                if (not std::exchange(warning_already_issued_cannot_open_user_config_file_, true)) {
                    log_warn("%s: could not open for writing: user settings will not be saved", user_config_path_->string().c_str());
                }
                return;
            }

            const toml::table settingsAsToml = to_toml_table(app_settings_lookup_);
            config_stream << c_config_file_header;
            config_stream << settingsAsToml;
            config_stream << '\n';

            is_dirty_ = false;
        }

    private:
        std::optional<std::filesystem::path> system_config_path_;
        std::optional<std::filesystem::path> user_config_path_;
        AppSettingsLookup app_settings_lookup_ = load_app_settings_lookup_from_disk(system_config_path_, user_config_path_);
        bool is_dirty_ = false;
        bool warning_already_issued_missing_user_config_ = false;
        bool warning_already_issued_cannot_open_user_config_file_ = false;
    };
}

class osc::AppSettings::Impl final {
public:
    Impl(
        std::string_view organization_name,
        std::string_view application_name,
        std::string_view application_config_file_name) :

        guarded_data_{organization_name, application_name, application_config_file_name}
    {}

    std::optional<std::filesystem::path> system_configuration_file_location() const
    {
        return guarded_data_.lock()->system_configuration_file_location();
    }

    std::optional<Variant> find_value(std::string_view key) const
    {
        return guarded_data_.lock()->find_value(key);
    }

    void set_value(std::string_view key, Variant value, AppSettingScope scope)
    {
        guarded_data_.lock()->set_value(key, std::move(value), scope);
    }

    void set_value_if_not_found(std::string_view key, Variant value, AppSettingScope scope)
    {
        guarded_data_.lock()->set_value_if_not_found(key, std::move(value), scope);
    }

    std::optional<std::filesystem::path> find_value_filesystem_source(
        std::string_view key) const
    {
        return guarded_data_.lock()->find_value_filesystem_source(key);
    }

    void sync()
    {
        guarded_data_.lock()->sync();
    }

private:
    SynchronizedValue<ThreadUnsafeAppSettings> guarded_data_;
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
            std::string_view organization_name,
            std::string_view application_name,
            std::string_view application_config_file_name)
        {
            auto [it, inserted] = data_.try_emplace(Key{organization_name, application_name, application_config_file_name});
            if (inserted) {
                it->second = std::make_shared<AppSettings::Impl>(organization_name, application_name, application_config_file_name);
            }
            return it->second;
        }
    private:
        using Key = std::tuple<std::string, std::string, std::string>;

        struct KeyHasher final {
            size_t operator()(const Key& k) const
            {
                return hash_of(std::get<0>(k), std::get<1>(k), std::get<2>(k));
            }
        };

        std::unordered_map<Key, std::shared_ptr<AppSettings::Impl>, KeyHasher> data_;
    };

    std::shared_ptr<AppSettings::Impl> get_globally_shared_settings_impl(
        std::string_view organization_name,
        std::string_view application_name,
        std::string_view application_config_file_name)
    {
        static SynchronizedValue<GlobalAppSettingsLookup> s_global_settings_lookup;
        return s_global_settings_lookup.lock()->get(organization_name, application_name, application_config_file_name);
    }

    std::filesystem::path get_resources_dir_fallback_path(const AppSettings& settings)
    {
        if (const auto system_config = settings.system_configuration_file_location()) {

            auto resources_dir_path = system_config->parent_path() / "resources";
            if (std::filesystem::exists(resources_dir_path)) {
                return resources_dir_path;
            }
            else {
                log_warn("resources path fallback: tried %s, but it doesn't exist", resources_dir_path.string().c_str());
            }
        }

        auto resources_relative_to_exe = current_executable_directory().parent_path() / "resources";
        if (std::filesystem::exists(resources_relative_to_exe)) {
            return resources_relative_to_exe;
        }
        else {
            log_warn("resources path fallback: using %s as a filler entry, but it doesn't actually exist: the application's configuration file has an incorrect/missing 'resources' key", resources_relative_to_exe.string().c_str());
        }

        return resources_relative_to_exe;
    }
}

std::filesystem::path osc::get_resource_dir_from_settings(const AppSettings& settings)
{
    // care: the resources directory is _very_, __very__ important
    //
    // if the application can't find resources, then it'll _probably_ fail to
    // boot correctly, which will result in great disappointment, so this code
    // has to try its best

    constexpr CStringView resources_key = "resources";

    const auto resources_dir_setting_value = settings.find_value(resources_key);
    if (not resources_dir_setting_value) {
        return get_resources_dir_fallback_path(settings);
    }

    if (resources_dir_setting_value->type() != VariantType::String) {
        log_error("application setting for '%s' is not a string: falling back", resources_key.c_str());
        return get_resources_dir_fallback_path(settings);
    }

    // resolve `resources` dir relative to the configuration file in which it was defined
    std::filesystem::path config_file_dir;
    if (const auto p = settings.find_value_filesystem_source(resources_key)) {
        config_file_dir = p->parent_path();
    }
    else {
        config_file_dir = current_executable_directory().parent_path();  // assume the `bin/` dir is one-up from the settings
    }
    const std::filesystem::path configured_resources_dir{resources_dir_setting_value->to<std::string>()};

    auto resolved_resource_dir = std::filesystem::weakly_canonical(config_file_dir / configured_resources_dir);
    if (not std::filesystem::exists(resolved_resource_dir)) {
        log_error("'resources', in the application configuration, points to a location that does not exist (%s), so the application may fail to load resources (which is usually a fatal error). Note: the 'resources' path is relative to the configuration file in which you define it (or can be absolute). Attemtping to fallback to a default resources location (which may or may not work).", resolved_resource_dir.string().c_str());
        return get_resources_dir_fallback_path(settings);
    }

    return resolved_resource_dir;
}

osc::AppSettings::AppSettings(
    std::string_view organization_name,
    std::string_view application_name,
    std::string_view application_config_file_name) :

    impl_{get_globally_shared_settings_impl(organization_name, application_name, application_config_file_name)}
{}
osc::AppSettings::AppSettings(const AppSettings&) = default;
osc::AppSettings::AppSettings(AppSettings&&) noexcept = default;
AppSettings& osc::AppSettings::operator=(const AppSettings&) = default;
AppSettings& osc::AppSettings::operator=(AppSettings&&) noexcept = default;
osc::AppSettings::~AppSettings() noexcept
{
    impl_->sync();
}

std::optional<std::filesystem::path> osc::AppSettings::system_configuration_file_location() const
{
    return impl_->system_configuration_file_location();
}

std::optional<Variant> osc::AppSettings::find_value(
    std::string_view key) const
{
    return impl_->find_value(key);
}

void osc::AppSettings::set_value(
    std::string_view key,
    Variant value,
    AppSettingScope scope)
{
    impl_->set_value(key, std::move(value), scope);
}

void osc::AppSettings::set_value_if_not_found(
    std::string_view key,
    Variant value,
    AppSettingScope scope)
{
    impl_->set_value_if_not_found(key, std::move(value), scope);
}

std::optional<std::filesystem::path> osc::AppSettings::find_value_filesystem_source(
    std::string_view key) const
{
    return impl_->find_value_filesystem_source(key);
}

void osc::AppSettings::sync()
{
    impl_->sync();
}
