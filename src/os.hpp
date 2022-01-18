#pragma once

#include "src/Log.hpp"

#include <filesystem>
#include <string_view>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osc
{
    // returns the full path to the currently-executing application
    std::filesystem::path const& CurrentExeDir();

    // returns the full path to the user's data directory
    std::filesystem::path const& GetUserDataDir();

    // writes a backtrace for the calling thread's stack to the log at the specified level
    void WriteTracebackToLog(log::level::LevelEnum);

    // installs a signal handler that prints a backtrace
    //
    // note: this is a noop on some OSes
    void InstallBacktraceHandler();

    // tries to open the specified filepath in the OSes default application for that
    // path. This function returns immediately: the application is opened in a separate
    // window.
    //
    // how, or what, the OS does is implementation-defined. E.g. Windows opens
    // filesystem paths by searching the file's extension against a list of default
    // applications. It opens URLs in the default browser, etc.
    void OpenPathInOSDefaultApplication(std::filesystem::path const&);

    // tries to open the specified URL in the OSes default browser
    //
    // how, or what, the OS does is implementation defined.
    void OpenURLInDefaultBrowser(std::string_view);

    // try to copy a string onto the user's clipboard
    bool SetClipboardText(char const*);

    // set an environment variable's value (process-wide)
    void SetEnv(char const* name, char const* value, bool overwrite);

    // synchronously prompt a user to select files ending with the supplied extensions (e.g. "obj,vtp,stl")
    //
    // - `extensions` can be nullptr, meaning "don't filter by extension"
    // - `extensions` can be a single extension (e.g. "blend")
    // - `extensions` can be a comma-delimited list of multiple extensions (e.g. "vtp,obj")
    // - `defaultPath` indicates which dir to initially open, can be nullptr, which will open a system-defined default
    std::vector<std::filesystem::path> PromptUserForFiles(char const* extensions, char const* defaultPath = nullptr);
}
