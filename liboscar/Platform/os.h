#pragma once

#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <string>
#include <utility>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osc
{
    // returns current system time as a calendar time
    std::tm system_calendar_time();

    // returns the full path to the currently-executing application
    //
    // care: can be slow: downstream callers should cache it
    std::filesystem::path current_executable_directory();

    // returns the full path to the user's data directory
    std::filesystem::path user_data_directory(
        std::string_view organization_name,
        std::string_view application_name
    );

    // tries to open the specified filepath in the OS's default application for opening
    // a path (usually, based on its extension)
    //
    // - this function returns immediately: the application is opened in a separate
    //   window
    //
    // - how, or what, the OS does is implementation-defined. E.g. on Windows, this
    //   opens the path by searching the file's extension against a list of default
    //   applications or, if Windows detects it's a URL, it will open the URL in
    //   the user's default web browser, etc.
    void open_file_in_os_default_application(const std::filesystem::path&);

    // tries to open the specified URL in the OS's default browser
    void open_url_in_os_default_web_browser(std::string_view);

    // returns the contents of the clipboard as text, or an empty string if nothing's in the clipboard
    std::string get_clipboard_text();

    // returns `true` if `content` was successfully copied to the user's clipboard
    bool set_clipboard_text(std::string_view);

    // sets an environment variable's value process-wide
    //
    // if `overwrite` is `true`, then it overwrites any previous value; otherwise,
    // it will only set the environment variable if no environment variable with
    // `name` exists
    void set_environment_variable(std::string_view name, std::string_view value, bool overwrite);

    // returns `true` if an environment variable with the given `name` is set in
    // the calling process.
    bool is_environment_variable_set(std::string_view name);

    // returns the content of an environment variable, if it's set. Otherwise, returns `std::nullopt`.
    std::optional<std::string> find_environment_variable(std::string_view name);

    // creates a temporary file in the most secure manner possible. There are no race conditions
    // in the file's creation - assuming that the operating system properly implements the `os.O_EXCL`
    // flag
    //
    // the caller is responsible for deleting the temporary file once it is no longer needed
    //
    // returns an open handle to the file (opened with `w+b`) and the absolute path to the temporary file
    std::pair<std::fstream, std::filesystem::path> mkstemp(
        std::string_view suffix = {},
        std::string_view prefix = {}
    );
}
