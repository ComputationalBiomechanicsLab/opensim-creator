#pragma once

#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osc
{
    // returns current system time as a calendar time
    std::tm system_calendar_time();

    // returns a `std::string` describing the current value of `errno`, but in
    // an implementation-defined threadsafe way
    std::string errno_to_string_threadsafe();

    // returns the full path to the currently-executing application
    //
    // care: can be slow: downstream callers should cache it
    std::filesystem::path current_executable_directory();

    // returns the full path to the user's data directory
    std::filesystem::path user_data_directory(
        std::string_view organization_name,
        std::string_view application_name
    );

    // calls the callback with each entry in the calling thread's stack
    void for_each_stacktrace_entry_in_this_thread(const std::function<void(std::string_view)>&);

    // installs a signal handler for crashes (SIGABRT/SIGSEGV, etc.) that will
    // print a thread backtrace to the process-wide log, followed by trying to
    // write a crash report as `CrashReport_DATE.txt` to `crash_dump_directory`
    void enable_crash_signal_backtrace_handler(const std::filesystem::path& crash_dump_directory);

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

    std::optional<std::filesystem::path> get_initial_directory_to_show_fallback();

    // Sets the directory that should be shown to the user if a call to one of the
    // `prompt_user*` files does not provide an `initial_directory_to_show`. If this
    // global fallback isn't provided, the implementation will fallback to whatever the
    // OS's default behavior is (typically, it remembers the user's last usage).
    //
    // This global fallback is activated until a call to `prompt_user*` is made without
    // the user cancelling out of the dialog (i.e. if the user cancels then this fallback
    // will remain in-place).
    void set_initial_directory_to_show_fallback(const std::filesystem::path&);
    void set_initial_directory_to_show_fallback(std::nullopt_t);  // reset it

    // synchronously prompt a user to select a file location for where to save a file
    //
    // - `maybe_extension` can be:
    //   - std::nullopt, meaning "don't filter by extension"
    //   - or a single extension (e.g. "blend")
    //   - (you can't use multiple extensions with this method)
    //
    // - `maybe_initial_directory_to_open` can be:
    //   - std::nullopt, meaning "use a system-defined default"
    //   - a directory to initially show to the user when the prompt opens
    //
    // - if the user manually types a filename without an extension (e.g. "model"), the implementation will add `extension`
    //   (if not std::nullopt) to the end of the user's string. It detects a lack of extension by searching the end of the user
    //   -supplied string for the given extension (if supplied)
    //
    // returns std::nullopt if the user doesn't select a file; otherwise, returns the user-selected save location--including the extension--if
    // the user selects a location
    std::optional<std::filesystem::path> prompt_user_for_file_save_location_add_extension_if_necessary(
        std::optional<std::string_view> maybe_extension = std::nullopt,
        std::optional<std::filesystem::path> maybe_initial_directory_to_open = std::nullopt
    );

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
