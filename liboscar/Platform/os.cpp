#include "os.h"

#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/ScopeExit.h>
#include <liboscar/Utils/StringHelpers.h>
#include <liboscar/Utils/SynchronizedValue.h>

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_time.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path convert_SDL_filepath_to_std_filepath(CStringView method_name, const char* p)
    {
        // nullptr disallowed
        if (p == nullptr) {
            std::stringstream ss;
            ss << method_name << ": returned null: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }

        std::string_view sv{p};

        // empty string disallowed
        if (sv.empty()) {
            std::stringstream ss;
            ss << method_name << ": returned an empty string";
            throw std::runtime_error{std::move(ss).str()};
        }

        // remove trailing slash: it interferes with `std::filesystem::path`
        if (sv.back() == '/' or sv.back() == '\\') {
            sv = sv.substr(0, sv.size()-1);
        }

        return std::filesystem::weakly_canonical(sv);
    }
}

std::tm osc::system_calendar_time()
{
    SDL_Time t{};
    SDL_GetCurrentTime(&t);

    SDL_DateTime dt{};
    SDL_TimeToDateTime(t, &dt, true);

    const auto doy = SDL_GetDayOfYear(dt.year, dt.month, dt.day);

    std::tm rv{};
    rv.tm_sec = dt.second;
    rv.tm_min = dt.minute;
    rv.tm_hour = dt.hour;
    rv.tm_mday = dt.day;
    rv.tm_mon = dt.month - 1;
    rv.tm_year = dt.year - 1900;
    rv.tm_wday = dt.day_of_week;
    rv.tm_yday = doy;
    rv.tm_isdst = dt.utc_offset > 0;
    rv.tm_gmtoff = dt.utc_offset;
    // rv.tm_zone  // TODO
    return rv;
}

std::filesystem::path osc::current_executable_directory()
{
    return convert_SDL_filepath_to_std_filepath("SDL_GetBasePath", SDL_GetBasePath());
}

std::filesystem::path osc::user_data_directory(
    std::string_view organization_name,
    std::string_view application_name)
{
    const std::string organization_name_str{organization_name};
    const std::string application_name_str{application_name};

    const std::unique_ptr<char, decltype(&SDL_free)> p{
        SDL_GetPrefPath(organization_name_str.c_str(), application_name_str.c_str()),
        SDL_free,
    };
    return convert_SDL_filepath_to_std_filepath("SDL_GetPrefPath", p.get());
}

void osc::open_file_in_os_default_application(const std::filesystem::path& fp)
{
    const std::string url = "file://" + std::filesystem::canonical(fp).string();
    open_url_in_os_default_web_browser(url);
}

void osc::open_url_in_os_default_web_browser(std::string_view url_view)
{
    const std::string url{url_view};
    if (SDL_OpenURL(url.c_str())) {
        log_info("opened %s", url.c_str());
    }
    else {
        log_error("could not open '%s': %s", url.c_str(), SDL_GetError());
    }
}

std::string osc::get_clipboard_text()
{
    if (char* str = SDL_GetClipboardText()) {
        const ScopeExit guard{[str]() { SDL_free(str); }};
        return str;
    }
    else {
        return {};
    }
}

bool osc::set_clipboard_text(std::string_view content)
{
    return SDL_SetClipboardText(std::string{content}.c_str());
}

void osc::set_environment_variable(std::string_view name, std::string_view value, bool overwrite)
{
    SDL_setenv_unsafe(std::string{name}.c_str(), std::string{value}.c_str(), overwrite ? 1 : 0);
}

bool osc::is_environment_variable_set(std::string_view name)
{
    return SDL_getenv_unsafe(std::string{name}.c_str()) != nullptr;
}

std::optional<std::string> osc::find_environment_variable(std::string_view name)
{
    if (const char* v = SDL_getenv_unsafe(std::string{name}.c_str())) {
        return std::string{v};
    }
    else {
        return std::nullopt;
    }
}

namespace
{
    constexpr std::string_view c_valid_dynamic_characters = "abcdefghijklmnopqrstuvwxyz0123456789";
    void write_dynamic_name_els(std::ostream& out)
    {
        std::default_random_engine s_prng{std::random_device{}()};
        std::array<char, 8> rv{};
        rgs::sample(c_valid_dynamic_characters, rv.begin(), rv.size(), s_prng);
        out << std::string_view{rv.begin(), rv.end()};
    }

    std::filesystem::path generate_tempfile_name(std::string_view suffix, std::string_view prefix)
    {
        std::stringstream ss;
        ss << prefix;
        write_dynamic_name_els(ss);
        ss << suffix;
        return std::filesystem::path{std::move(ss).str()};
    }
}

std::pair<std::fstream, std::filesystem::path> osc::mkstemp(std::string_view suffix, std::string_view prefix)
{
    const std::filesystem::path tmpdir = std::filesystem::temp_directory_path();
    for (size_t attempt = 0; attempt < 100; ++attempt) {
        std::filesystem::path attempt_path = tmpdir / generate_tempfile_name(suffix, prefix);
        // TODO: remove these `pragma`s once the codebase is upgraded to C++23, because it has `std::ios_base::noreplace` support
#pragma warning(push)
#pragma warning(suppress : 4996)
        if (auto fd = std::fopen(attempt_path.string().c_str(), "w+x"); fd != nullptr) {  // TODO: replace with `std::ios_base::noreplace` in C++23 (waiting on XCode16)
            std::fclose(fd);
            return {std::fstream{attempt_path, std::ios_base::in | std::ios_base::out | std::ios_base::binary}, std::move(attempt_path)};
        }
#pragma warning(pop)
    }
    throw std::runtime_error{"failed to create a unique temporary filename after 100 attempts - are you creating _a lot_ of temporary files? ;)"};
}
