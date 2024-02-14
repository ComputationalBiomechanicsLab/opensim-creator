#include "os.h"

#include <oscar/Platform/Log.h>
#include <oscar/Platform/LogLevel.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/StringHelpers.h>

#include <nfd.h>
#include <SDL_clipboard.h>
#include <SDL_error.h>
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    std::filesystem::path convertSDLPathToStdpath(CStringView methodname, char* p)
    {
        // nullptr disallowed
        if (p == nullptr)
        {
            std::stringstream ss;
            ss << methodname << ": returned null: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }

        std::string_view sv{p};

        // empty string disallowed
        if (sv.empty())
        {
            std::stringstream ss;
            ss << methodname << ": returned an empty string";
            throw std::runtime_error{std::move(ss).str()};
        }

        // remove trailing slash: it interferes with std::filesystem::path
        if (sv.back() == '/' || sv.back() == '\\')
        {
            sv = sv.substr(0, sv.size()-1);
        }

        return std::filesystem::weakly_canonical(sv);
    }
}

std::tm osc::GetSystemCalendarTime()
{
    std::chrono::system_clock::time_point const tp = std::chrono::system_clock::now();
    std::time_t const t = std::chrono::system_clock::to_time_t(tp);
    return GMTimeThreadsafe(t);
}

std::filesystem::path osc::CurrentExeDir()
{
    std::unique_ptr<char, decltype(&SDL_free)> p
    {
        SDL_GetBasePath(),
        SDL_free
    };
    return convertSDLPathToStdpath("SDL_GetBasePath", p.get());
}

std::filesystem::path osc::GetUserDataDir(
    CStringView organizationName,
    CStringView applicationName)
{
    std::unique_ptr<char, decltype(&SDL_free)> p
    {
        SDL_GetPrefPath(organizationName.c_str(), applicationName.c_str()),
        SDL_free,
    };
    return convertSDLPathToStdpath("SDL_GetPrefPath", p.get());
}

bool osc::SetClipboardText(CStringView sv)
{
    return SDL_SetClipboardText(sv.c_str()) == 0;
}

void osc::SetEnv(CStringView name, CStringView value, bool overwrite)
{
    SDL_setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0);
}

std::optional<std::filesystem::path> osc::PromptUserForFile(
    std::optional<CStringView> maybeCommaDelimitedExtensions,
    std::optional<CStringView> maybeInitialDirectoryToOpen)
{
    auto [path, result] = [&]()
    {
        nfdchar_t* ptr = nullptr;
        nfdresult_t const res = NFD_OpenDialog(
            maybeCommaDelimitedExtensions ? maybeCommaDelimitedExtensions->c_str() : nullptr,
            maybeInitialDirectoryToOpen ? maybeInitialDirectoryToOpen->c_str() : nullptr,
            &ptr
        );
        return std::pair<std::unique_ptr<nfdchar_t, decltype(::free)*>, nfdresult_t>
        {
            std::unique_ptr<nfdchar_t, decltype(::free)*>{ptr, ::free},
            res,
        };
    }();

    if (path && result == NFD_OKAY)
    {
        static_assert(std::is_same_v<nfdchar_t, char>);
        return std::filesystem::weakly_canonical(path.get());
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<std::filesystem::path> osc::PromptUserForFiles(
    std::optional<CStringView> maybeCommaDelimitedExtensions,
    std::optional<CStringView> maybeInitialDirectoryToOpen)
{
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple(
        maybeCommaDelimitedExtensions ? maybeCommaDelimitedExtensions->c_str() : nullptr,
        maybeInitialDirectoryToOpen ? maybeInitialDirectoryToOpen->c_str() : nullptr,
        &s
    );

    std::vector<std::filesystem::path> rv;

    if (result == NFD_OKAY)
    {
        size_t len = NFD_PathSet_GetCount(&s);
        rv.reserve(len);
        for (size_t i = 0; i < len; ++i)
        {
            rv.emplace_back(std::filesystem::weakly_canonical(NFD_PathSet_GetPath(&s, i)));
        }

        NFD_PathSet_Free(&s);
    }
    else if (result == NFD_CANCEL)
    {
    }
    else
    {
        log_error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
    }

    return rv;
}

std::optional<std::filesystem::path> osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary(
    std::optional<CStringView> maybeExtension,
    std::optional<CStringView> maybeInitialDirectoryToOpen)
{
    if (maybeExtension)
    {
        OSC_ASSERT(!Contains(*maybeExtension, ',') && "can only provide one extension to this implementation!");
    }

    auto [path, result] = [&]()
    {
        nfdchar_t* ptr = nullptr;
        nfdresult_t const res = NFD_SaveDialog(
            maybeExtension ? maybeExtension->c_str() : nullptr,
            maybeInitialDirectoryToOpen ? maybeInitialDirectoryToOpen->c_str() : nullptr,
            &ptr
        );
        return std::pair<std::unique_ptr<nfdchar_t, decltype(::free)*>, nfdresult_t>
        {
            std::unique_ptr<nfdchar_t, decltype(::free)*>{ptr, ::free},
            res,
        };
    }();

    if (result != NFD_OKAY)
    {
        return std::nullopt;
    }

    static_assert(std::is_same_v<nfdchar_t, char>);
    auto p = std::filesystem::weakly_canonical(path.get());

    if (maybeExtension)
    {
        // ensure that the user-selected path is tested against '.$EXTENSION' (#771)
        //
        // the caller only provides the extension without the dot (this is what
        // NFD requires) but the user may have manually wrote a string that is
        // suffixed with the dot-less version of the extension (e.g. "somecsv")

        std::string const fullExtension = std::string{"."} + *maybeExtension;
        if (!std::string_view{path.get()}.ends_with(fullExtension))
        {
            p += fullExtension;
        }
    }

    return p;
}

std::string osc::CurrentErrnoAsString()
{
    return StrerrorThreadsafe(errno);
}


#ifdef __LINUX__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <oscar/Shims/Cpp20/bit.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <execinfo.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using osc::log_error;

std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv{};
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    std::array<char, 1024> buf{};

    auto* maybeErr = strerror_r(errnum, buf.data(), buf.size());
    if (std::is_same_v<int, decltype(maybeErr)> && !maybeErr)
    {
        log_warn("a call to strerror_r failed with error code %i", maybeErr);
        return {};
    }
    else
    {
        static_cast<void>(maybeErr);
    }

    std::string rv{buf.data()};
    if (rv.size() == buf.size())
    {
        log_warn("a call to strerror_r returned an error string that was as big as the buffer: an OS error message may have been truncated!");
    }
    return rv;
}

void osc::WriteTracebackToLog(LogLevel lvl)
{
    std::array<void*, 50> ary{};
    int const size = backtrace(ary.data(), ary.size());
    std::unique_ptr<char*, decltype(::free)*> messages{backtrace_symbols(ary.data(), size), ::free};

    if (!messages)
    {
        return;
    }

    for (int i = 0; i < size; ++i)
    {
        log_message(lvl, "%s", messages.get()[i]);
    }
}

namespace
{
    /* This structure mirrors the one found in /usr/include/asm/ucontext.h */
    struct osc_sig_ucontext final {
        unsigned long uc_flags;  // NOLINT(google-runtime-int)
        ucontext_t* uc_link;
        stack_t uc_stack;
        sigcontext uc_mcontext;
        sigset_t uc_sigmask;
    };

    void OnCriticalSignalRecv(int sig_num, siginfo_t* info, void* ucontext)
    {
        // reset abort signal handler
        if (signal(SIGABRT, SIG_DFL) == SIG_ERR)
        {
            log_error("failed to reset SIGABRT handler - the program may not be able to crash correctly");
        }

        // reset segfault signal handler
        if (signal(SIGSEGV, SIG_DFL) == SIG_ERR)
        {
            log_error("failed to reset SIGSEGV handler - the program may not be able to crash correctly");
        }

        auto* uc = static_cast<osc_sig_ucontext*>(ucontext);

        /* Get the address at the time the signal was raised */
#if defined(__i386__)  // gcc specific
        void* caller_address = cpp20::bit_cast<void*>(uc->uc_mcontext.eip);  // EIP: x86 specific
#elif defined(__x86_64__)  // gcc specific
        void* callerAddress = cpp20::bit_cast<void*>(uc->uc_mcontext.rip);  // RIP: x86_64 specific
#else
#error Unsupported architecture.
#endif

        std::cerr << "critical error: signal " << sig_num << '(' << strsignal(sig_num) << ") received from OS: address is " <<  info->si_addr << " from " << callerAddress;  // NOLINT(concurrency-mt-unsafe)

        std::array<void*, 50> ary{};
        int const size = backtrace(ary.data(), ary.size());
        ary[1] = callerAddress;  // overwrite sigaction with caller's address

        std::unique_ptr<char*, decltype(::free)*> const messages{backtrace_symbols(ary.data(), size), ::free};
        if (!messages)
        {
            return;
        }

        /* skip first stack frame (points here) */
        for (int i = 1; i < size; ++i)
        {
            std::cerr << "    #" << std::setw(2) << i << ' ' << messages.get()[i] << '\n';
        }
    }
}

void osc::InstallBacktraceHandler(std::filesystem::path const&)
{
    struct sigaction sigact{};

    sigact.sa_sigaction = OnCriticalSignalRecv;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    // install segfault handler
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        log_error("could not set signal handler for SIGSEGV: error reporting may not work as intended");
    }

    // install abort handler: this triggers whenever a non-throwing `assert` causes a termination
    if (sigaction(SIGABRT, &sigact, nullptr) != 0)
    {
        log_error("could not set signal handler for SIGABRT: error reporting may not work as intended");
    }
}

void osc::OpenPathInOSDefaultApplication(std::filesystem::path const& fp)
{
    // fork a subprocess
    pid_t pid = fork();

    if (pid == -1)
    {
        // failed to fork a process
        log_error("failed to fork() a new subprocess: this usually only happens if you have unusual OS settings: see 'man fork' ERRORS for details");
        return;
    }
    else if (pid != 0)
    {
        // fork successful and this thread is inside the parent
        //
        // have the parent thread `wait` for the child thread to finish
        // what it's doing (xdg-open, itself, forks + detaches)
        log_info("fork()ed a subprocess for 'xdg-open %s'", fp.c_str());

        int rv = 0;
        waitpid(pid, &rv, 0);

        if (rv)
        {
            log_error("fork()ed subprocess returned an error code of %i", rv);
        }

        return;
    }
    else
    {
        // fork successful and we're inside the child
        //
        // immediately `exec` into `xdg-open`, which will aggro-replace this process
        // image (+ this thread) with xdg-open
        int rv = execlp("xdg-open", "xdg-open", fp.c_str(), static_cast<char*>(nullptr));

        // this thread only reaches here if there is some kind of error in `exec`
        //
        // aggressively exit this thread, returning the status code. Do **not**
        // return from this thread, because it should'nt behave as-if it were
        // the calling thread
        //
        // use `_exit`, rather than `exit`, because we don't want the fork to
        // potentially call `atexit` handlers that screw the parent (#627)
        _exit(rv);
    }
}

void osc::OpenURLInDefaultBrowser(std::string_view vw)
{
    // (we know that xdg-open handles this automatically)
    OpenPathInOSDefaultApplication(vw);
}

#elif defined(__APPLE__)
#include <errno.h>  // ERANGE
#include <execinfo.h>  // backtrace(), backtrace_symbols()
#include <signal.h>  // sigaction(), struct sigaction, strsignal()
#include <stdlib.h>  // exit(), free()
#include <string.h>  // strerror_r()
#include <time.h>  // gmtime_r()

using osc::log_error;
using osc::log_message;
using osc::log_warn;

std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    std::array<char, 512> buf{};
    if (strerror_r(errnum, buf.data(), buf.size()) == ERANGE)
    {
        log_warn("a call to strerror_r returned ERANGE: an OS error message may have been truncated!");
    }
    return std::string{buf.data()};
}


void osc::WriteTracebackToLog(LogLevel lvl)
{
    void* array[50];
    int size = backtrace(array, 50);
    std::unique_ptr<char*, decltype(::free)*> messages{backtrace_symbols(array, size), ::free};

    if (!messages)
    {
        return;
    }

    for (int i = 0; i < size; ++i)
    {
        log_message(lvl, "%s", messages.get()[i]);
    }
}

namespace
{
    [[noreturn]] void OSC_critical_error_handler(int sig_num, siginfo_t*, void*)
    {
        log_error("critical error: signal %d (%s) received from OS", sig_num, strsignal(sig_num));
        log_error("backtrace:");
        osc::WriteTracebackToLog(osc::LogLevel::err);
        exit(EXIT_FAILURE);
    }
}

void osc::InstallBacktraceHandler(std::filesystem::path const&)
{
    struct sigaction sigact;
    sigact.sa_sigaction = OSC_critical_error_handler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    // enable SIGSEGV (segmentation fault) handler
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        log_warn("could not set a signal handler for SIGSEGV: crash error reporting may not work as intended");
    }

    // enable SIGABRT (abort) handler - usually triggers when `assert` fails or std::terminate is called
    if (sigaction(SIGABRT, &sigact, nullptr) != 0)
    {
        log_warn("could not set a signal handler for SIGABRT: crash error reporting may not work as intended");
    }
}

void osc::OpenPathInOSDefaultApplication(std::filesystem::path const& p)
{
    std::string cmd = "open " + p.string();
    system(cmd.c_str());
}

void osc::OpenURLInDefaultBrowser(std::string_view url)
{
    std::string cmd = "open " + std::string{url};
    system(cmd.c_str());
}

#elif defined(WIN32)
#include <Windows.h>  // PVOID, RtlCaptureStackBackTrace(), MEMORY_BASIC_INFORMATION, VirtualQuery(), DWORD64, TCHAR, GetModuleFileName()
#include <time.h>  // gmtime_s
#include <cinttypes>  // PRIXPTR
#include <signal.h>   // signal()

#include <oscar/Shims/Cpp20/bit.h>

using osc::defaultLogger;
using osc::getTracebackLog;
using osc::LogMessage;
using osc::LogMessageView;
using osc::LogSink;
using osc::ToCStringView;

std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_s(&rv, &t);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    std::array<char, 512> buf{};
    if (errno_t rv = strerror_s(buf.data(), buf.size(), errnum); rv != 0)
    {
        log_warn("a call to strerror_s returned an error (%i): an OS error message may be missing!", rv);
    }
    return std::string{buf.data()};
}

void osc::WriteTracebackToLog(LogLevel lvl)
{
    constexpr size_t skipped_frames = 0;
    constexpr size_t num_frames = 16;

    PVOID return_addrs[num_frames];

    // popupate [0, n) with return addresses (see MSDN)
    USHORT n = RtlCaptureStackBackTrace(skipped_frames, num_frames, return_addrs, nullptr);

    log_message(lvl, "backtrace:");
    for (size_t i = 0; i < n; ++i)
    {
        // figure out where the address is relative to the start of the page range the address
        // falls in (effectively, where it is relative to the start of the memory-mapped DLL/exe)
        MEMORY_BASIC_INFORMATION bmi;
        VirtualQuery(return_addrs[i], &bmi, sizeof(bmi));
        DWORD64 base_addr = cpp20::bit_cast<DWORD64>(bmi.AllocationBase);
        static_assert(sizeof(DWORD64) == 8 && sizeof(PVOID) == 8, "review this code - might not work so well on 32-bit systems");

        // use the base address to figure out the file name
        TCHAR module_namebuf[1024];
        GetModuleFileName(cpp20::bit_cast<HMODULE>(base_addr), module_namebuf, 1024);

        // find the final element in the filename
        TCHAR* cursor = module_namebuf;
        TCHAR* filename_start = cursor;
        while (*cursor != '\0')
        {
            if (*cursor == '\\')
            {
                filename_start = cursor + 1;  // skip the slash
            }
            ++cursor;
        }

        PVOID relative_addr = cpp20::bit_cast<PVOID>(cpp20::bit_cast<DWORD64>(return_addrs[i]) - base_addr);

        log_message(lvl, "    #%zu %s+0x%" PRIXPTR " [0x%" PRIXPTR "]", i, filename_start, (uintptr_t)relative_addr, (uintptr_t)return_addrs[i]);
    }
    log_message(lvl, "note: backtrace addresses are return addresses, not call addresses (see: https://devblogs.microsoft.com/oldnewthing/20170505-00/?p=96116)");
    log_message(lvl, "to analyze the backtrace in WinDbg: `ln application.exe+ADDR`");

    // in windbg: ln osc.exe+ADDR
    // viewing it: https://stackoverflow.com/questions/54022914/c-is-there-any-command-likes-addr2line-on-windows
}

namespace
{
    // temporarily attach a crash logger to the log
    class CrashFileSink final : public LogSink {
    public:
        explicit CrashFileSink(std::ostream& out_) : m_Out{out_}
        {
        }

    private:
        void implLog(LogMessageView const& msg) final
        {
            if (m_Out)
            {
                m_Out << '[' << msg.loggerName << "] [" << ToCStringView(msg.level) << "] " << msg.payload << std::endl;
            }
        }

        std::ostream& m_Out;
    };

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds GetCurrentTimeAsUnixTimestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    // care: this is necessary because segfault crash handlers don't appear to
    // be able to have data passed to them
    std::optional<std::filesystem::path> UpdCrashDumpDirGlobal()
    {
        static std::optional<std::filesystem::path> s_CrashDumpDir;
        return s_CrashDumpDir;
    }

    std::optional<std::filesystem::path> GetCrashReportPath()
    {
        if (!UpdCrashDumpDirGlobal())
        {
            return std::nullopt;  // global wasn't set: programmer error
        }

        std::stringstream filename;
        filename << GetCurrentTimeAsUnixTimestamp().count();
        filename << "_CrashReport.txt";
        return *UpdCrashDumpDirGlobal() / std::move(filename).str();
    }

    LONG crash_handler(EXCEPTION_POINTERS*)
    {
        log_error("exception propagated to root of the application: might be a segfault?");

        std::optional<std::filesystem::path> const maybeCrashReportPath =
            GetCrashReportPath();

        std::optional<std::ofstream> maybeCrashReportFile = maybeCrashReportPath ?
            std::ofstream{*maybeCrashReportPath} :
            std::optional<std::ofstream>{};

        // dump out the log history (it's handy for context)
        if (maybeCrashReportFile && *maybeCrashReportFile)
        {
            *maybeCrashReportFile << "----- log -----\n";
            auto guard = getTracebackLog().lock();
            for (LogMessage const& msg : *guard)
            {
                *maybeCrashReportFile << '[' << msg.loggerName << "] [" << ToCStringView(msg.level) << "] " << msg.payload << '\n';
            }
            *maybeCrashReportFile << "----- /log -----\n";
        }

        // then write a traceback to both the log (in case the user is running from a console)
        // *and* the crash dump (in case the user is running from a GUI and wants to report it)
        if (maybeCrashReportFile && *maybeCrashReportFile)
        {
            *maybeCrashReportFile << "----- traceback -----\n";

            std::shared_ptr<osc::LogSink> sink = std::make_shared<CrashFileSink>(*maybeCrashReportFile);

            defaultLogger()->sinks().push_back(sink);
            WriteTracebackToLog(osc::LogLevel::err);
            defaultLogger()->sinks().erase(defaultLogger()->sinks().end() - 1);

            *maybeCrashReportFile << "----- /traceback -----\n";
        }
        else
        {
            // (no crash dump file, but still write it to stdout etc.)

            *maybeCrashReportFile << "----- traceback -----\n";
            WriteTracebackToLog(osc::LogLevel::err);
            *maybeCrashReportFile << "----- /traceback -----\n";
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void signal_handler(int)
    {
        log_error("signal caught by application: printing backtrace");
        WriteTracebackToLog(osc::LogLevel::err);
    }
}

void osc::InstallBacktraceHandler(std::filesystem::path const& crashDumpDir)
{
    // https://stackoverflow.com/questions/13591334/what-actions-do-i-need-to-take-to-get-a-crash-dump-in-all-error-scenarios

    // set crash dump directory globally so that the crash handler can see it
    UpdCrashDumpDirGlobal() = crashDumpDir;

    // system default: display all errors
    SetErrorMode(0);

    // when the application crashes due to an exception, call this handler
    SetUnhandledExceptionFilter(crash_handler);

    signal(SIGABRT, signal_handler);
}

void osc::OpenPathInOSDefaultApplication(std::filesystem::path const& p)
{
    ShellExecute(0, 0, p.string().c_str(), 0, 0 , SW_SHOW);
}

void osc::OpenURLInDefaultBrowser(std::string_view)
{
    log_error("unsupported action: cannot open external URLs in Windows (yet!)");
}
#else
#error "Unsupported platform?"
#endif
