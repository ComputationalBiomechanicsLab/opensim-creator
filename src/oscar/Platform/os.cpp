#include "os.hpp"

#include "oscar/Platform/Log.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/ScopeGuard.hpp"
#include "oscar/Utils/StringHelpers.hpp"

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
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    std::filesystem::path convertSDLPathToStdpath(char const* methodname, char* p)
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

        return std::filesystem::path{sv};
    }

    std::filesystem::path getCurrentExeDir()
    {
        std::unique_ptr<char, decltype(&SDL_free)> p{SDL_GetBasePath(), SDL_free};

        return convertSDLPathToStdpath("SDL_GetBasePath", p.get());
    }

    std::filesystem::path getUserDataDir()
    {
        std::unique_ptr<char, decltype(&SDL_free)> p{SDL_GetPrefPath("cbl", "osc"), SDL_free};

        return convertSDLPathToStdpath("SDL_GetPrefPath", p.get());
    }
}

std::tm osc::GetSystemCalendarTime()
{
    std::chrono::system_clock::time_point const tp = std::chrono::system_clock::now();
    std::time_t const t = std::chrono::system_clock::to_time_t(tp);
    return osc::GMTimeThreadsafe(t);
}

std::filesystem::path const& osc::CurrentExeDir()
{
    // can be expensive to compute: cache after first retrieval
    static std::filesystem::path const s_CurrentExeDir = getCurrentExeDir();
    return s_CurrentExeDir;
}

std::filesystem::path const& osc::GetUserDataDir()
{
    // can be expensive to compute: cache after first retrieval
    static std::filesystem::path const s_UserDataDir = getUserDataDir();
    return s_UserDataDir;
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
    nfdchar_t* path = nullptr;
    nfdresult_t const result = NFD_OpenDialog(
        maybeCommaDelimitedExtensions ? maybeCommaDelimitedExtensions->c_str() : nullptr,
        maybeInitialDirectoryToOpen ? maybeInitialDirectoryToOpen->c_str() : nullptr,
        &path
    );
    OSC_SCOPE_GUARD_IF(path, { free(path); });

    if (path && result == NFD_OKAY)
    {
        return std::filesystem::path{path};
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
            rv.emplace_back(NFD_PathSet_GetPath(&s, i));
        }

        NFD_PathSet_Free(&s);
    }
    else if (result == NFD_CANCEL)
    {
    }
    else
    {
        log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
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

    nfdchar_t* path = nullptr;
    nfdresult_t const result = NFD_SaveDialog(
        maybeExtension ? maybeExtension->c_str() : nullptr,
        maybeInitialDirectoryToOpen ? maybeInitialDirectoryToOpen->c_str() : nullptr,
        &path
    );
    OSC_SCOPE_GUARD_IF(path, { free(path); });

    if (result != NFD_OKAY)
    {
        return std::nullopt;
    }

    std::filesystem::path p{path};

    if (maybeExtension && !CStrEndsWith(path, *maybeExtension))
    {
        p += '.';
        p += std::string_view{*maybeExtension};
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

#include <cerrno>  // ERANGE
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <execinfo.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv{};
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    std::array<char, 1024> buf{};

    // ignore return value because strerror_r has two versions in
    // Linux and the GNU version doesn't return a useful error code
    auto maybeErr = strerror_r(errnum, buf.data(), buf.size());

    if (std::is_same_v<int, decltype(maybeErr)> && !maybeErr)
    {
        osc::log::warn("a call to strerror_r failed with error code %i", maybeErr);
        return {};
    }
    else
    {
        (void)maybeErr;
    }

    std::string rv{buf.data()};
    if (rv.size() == buf.size())
    {
        osc::log::warn("a call to strerror_r returned an error string that was as big as the buffer: an OS error message may have been truncated!");
    }
    return rv;
}

void osc::WriteTracebackToLog(log::Level lvl)
{
    std::array<void*, 50> ary{};
    int const size = backtrace(ary.data(), ary.size());
    char** const messages = backtrace_symbols(ary.data(), size);

    if (messages == nullptr)
    {
        return;
    }
    OSC_SCOPE_GUARD({ free(messages); });

    for (size_t i = 0; i < size; ++i)
    {
        osc::log::log(lvl, "%s", messages[i]);
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
}

static void OnCriticalSignalRecv(int sig_num, siginfo_t* info, void* ucontext)
{
    // reset abort signal handler
    if (signal(SIGABRT, SIG_DFL) == SIG_ERR)
    {
        osc::log::error("failed to reset SIGABRT handler - the program may not be able to crash correctly");
    }

    // reset segfault signal handler
    if (signal(SIGSEGV, SIG_DFL) == SIG_ERR)
    {
        osc::log::error("failed to reset SIGSEGV handler - the program may not be able to crash correctly");
    }

    auto* uc = static_cast<osc_sig_ucontext*>(ucontext);

    /* Get the address at the time the signal was raised */
#if defined(__i386__)  // gcc specific
    void* caller_address = reinterpret_cast<void*>(uc->uc_mcontext.eip);  // EIP: x86 specific
#elif defined(__x86_64__)  // gcc specific
    void* callerAddress = reinterpret_cast<void*>(uc->uc_mcontext.rip);  // RIP: x86_64 specific
#else
#error Unsupported architecture.
#endif

    std::cerr << "osc: critical error: signal " << sig_num << '(' << strsignal(sig_num) << ") received from OS: address is " <<  info->si_addr << " from " << callerAddress;  // NOLINT(concurrency-mt-unsafe)

    std::array<void*, 50> ary{};
    int const size = backtrace(ary.data(), ary.size());
    ary[1] = callerAddress;  // overwrite sigaction with caller's address

    char** const messages = backtrace_symbols(ary.data(), size);

    if (messages == nullptr)
    {
        return;
    }
    OSC_SCOPE_GUARD({ free(messages); });

    /* skip first stack frame (points here) */
    for (int i = 1; i < size; ++i)
    {
        std::cerr << "    #" << std::setw(2) << i << ' ' << messages[i] << '\n';
    }
}

void osc::InstallBacktraceHandler()
{
    struct sigaction sigact{};

    sigact.sa_sigaction = OnCriticalSignalRecv;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    // install segfault handler
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        osc::log::error("osc: warning: could not set signal handler for SIGSEGV: error reporting may not work as intended");
    }

    // install abort handler: this triggers whenever a non-throwing `assert` causes a termination
    if (sigaction(SIGABRT, &sigact, nullptr) != 0)
    {
        osc::log::error("osc: warning: could not set signal handler for SIGABRT: error reporting may not work as intended");
    }
}

void osc::OpenPathInOSDefaultApplication(std::filesystem::path const& fp)
{
    // fork a subprocess
    pid_t pid = fork();

    if (pid == -1)
    {
        // failed to fork a process
        log::error("failed to fork() a new subprocess: this usually only happens if you have unusual OS settings: see 'man fork' ERRORS for details");
        return;
    }
    else if (pid != 0)
    {
        // fork successful and this thread is inside the parent
        //
        // have the parent thread `wait` for the child thread to finish
        // what it's doing (xdg-open, itself, forks + detaches)
        log::info("fork()ed a subprocess for 'xdg-open %s'", fp.c_str());

        int rv = 0;
        waitpid(pid, &rv, 0);

        if (rv)
        {
            log::error("fork()ed subprocess returned an error code of %i", rv);
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

std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    char buf[512];
    if (strerror_r(errnum, buf, sizeof(buf)) == ERANGE)
    {
        osc::log::warn("a call to strerror_r returned ERANGE: an OS error message may have been truncated!");
    }
    return std::string{buf};
}


void osc::WriteTracebackToLog(log::Level lvl)
{
    void* array[50];
    int size = backtrace(array, 50);
    char** messages = backtrace_symbols(array, size);

    if (messages == nullptr)
    {
        return;
    }

    OSC_SCOPE_GUARD({ free(messages); });

    for (int i = 0; i < size; ++i)
    {
        osc::log::log(lvl, "%s", messages[i]);
    }
}

namespace
{
    [[noreturn]] void OSC_critical_error_handler(int sig_num, siginfo_t* info, void* ucontext)
    {
        osc::log::error("critical error: signal %d (%s) received from OS", sig_num, strsignal(sig_num));
        osc::log::error("backtrace:");
        osc::WriteTracebackToLog(osc::log::Level::err);
        exit(EXIT_FAILURE);
    }
}

void osc::InstallBacktraceHandler()
{
    struct sigaction sigact;
    sigact.sa_sigaction = OSC_critical_error_handler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    // enable SIGSEGV (segmentation fault) handler
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        log::warn("could not set a signal handler for SIGSEGV: crash error reporting may not work as intended");
    }

    // enable SIGABRT (abort) handler - usually triggers when `assert` fails or std::terminate is called
    if (sigaction(SIGABRT, &sigact, nullptr) != 0)
    {
        log::warn("could not set a signal handler for SIGABRT: crash error reporting may not work as intended");
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

std::tm osc::GMTimeThreadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_s(&rv, &t);
    return rv;
}

std::string osc::StrerrorThreadsafe(int errnum)
{
    char buf[512];
    if (errno_t rv = strerror_s(buf, sizeof(buf), errnum); rv != 0 )
    {
        osc::log::warn("a call to strerror_s returned an error (%i): an OS error message may be missing!", rv);
    }
    return std::string{buf};
}

void osc::WriteTracebackToLog(log::Level lvl)
{
    constexpr size_t skipped_frames = 0;
    constexpr size_t num_frames = 16;

    PVOID return_addrs[num_frames];

    // popupate [0, n) with return addresses (see MSDN)
    USHORT n = RtlCaptureStackBackTrace(skipped_frames, num_frames, return_addrs, nullptr);

    log::log(lvl, "backtrace:");
    for (size_t i = 0; i < n; ++i)
    {
        // figure out where the address is relative to the start of the page range the address
        // falls in (effectively, where it is relative to the start of the memory-mapped DLL/exe)
        MEMORY_BASIC_INFORMATION bmi;
        VirtualQuery(return_addrs[i], &bmi, sizeof(bmi));
        DWORD64 base_addr = reinterpret_cast<DWORD64>(bmi.AllocationBase);
        static_assert(sizeof(DWORD64) == 8 && sizeof(PVOID) == 8, "review this code - might not work so well on 32-bit systems");

        // use the base address to figure out the file name
        TCHAR module_namebuf[1024];
        GetModuleFileName(reinterpret_cast<HMODULE>(base_addr), module_namebuf, 1024);

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

        PVOID relative_addr = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(return_addrs[i]) - base_addr);

        log::log(lvl, "    #%zu %s+0x%" PRIXPTR " [0x%" PRIXPTR "]", i, filename_start, (uintptr_t)relative_addr, (uintptr_t)return_addrs[i]);
    }
    log::log(lvl, "note: backtrace addresses are return addresses, not call addresses (see: https://devblogs.microsoft.com/oldnewthing/20170505-00/?p=96116)");
    log::log(lvl, "to analyze the backtrace in WinDbg: `ln osc.exe+ADDR`");

    // in windbg: ln osc.exe+ADDR
    // viewing it: https://stackoverflow.com/questions/54022914/c-is-there-any-command-likes-addr2line-on-windows
}

namespace
{
    // temporarily attach a crash logger to the log
    class CrashFileSink final : public osc::log::Sink {
    public:
        explicit CrashFileSink(std::ostream& out_) : m_Out{out_}
        {
        }

    private:
        void implLog(osc::log::LogMessage const& msg) final
        {
            if (m_Out)
            {
                m_Out << '[' << msg.loggerName << "] [" << osc::log::toCStringView(msg.level) << "] " << msg.payload << std::endl;
            }
        }

        std::ostream& m_Out;
    };

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds GetCurrentTimeAsUnixTimestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    std::filesystem::path GetCrashReportPath()
    {
        std::stringstream filename;
        filename << GetCurrentTimeAsUnixTimestamp().count();
        filename << "_CrashReport.txt";
        return osc::GetUserDataDir() / std::move(filename).str();
    }

    LONG crash_handler(EXCEPTION_POINTERS*)
    {
        osc::log::error("exception propagated to root of OSC: might be a segfault?");

        std::filesystem::path const crashReportPath = GetCrashReportPath();
        std::ofstream crashReportFile{crashReportPath};

        // dump out the log history (it's handy for context)
        if (crashReportFile)
        {
            crashReportFile << "----- log -----\n";
            auto guard = osc::log::getTracebackLog().lock();
            for (osc::log::OwnedLogMessage const& msg : *guard)
            {
                crashReportFile << '[' << msg.loggerName << "] [" << osc::log::toCStringView(msg.level) << "] " << msg.payload << '\n';
            }
            crashReportFile << "----- /log -----\n";
        }

        // then write a traceback to both the log (in case the user is running from a console)
        // *and* the crash dump (in case the user is running from a GUI and wants to report it)
        if (crashReportFile)
        {
            crashReportFile << "----- traceback -----\n";

            std::shared_ptr<osc::log::Sink> sink = std::make_shared<CrashFileSink>(crashReportFile);

            osc::log::defaultLogger()->sinks().push_back(sink);
            osc::WriteTracebackToLog(osc::log::Level::err);
            osc::log::defaultLogger()->sinks().erase(osc::log::defaultLogger()->sinks().end() - 1);

            crashReportFile << "----- /traceback -----\n";
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void signal_handler(int)
    {
        osc::log::error("signal caught by OSC: printing backtrace");
        osc::WriteTracebackToLog(osc::log::Level::err);
    }
}

void osc::InstallBacktraceHandler()
{
    // https://stackoverflow.com/questions/13591334/what-actions-do-i-need-to-take-to-get-a-crash-dump-in-all-error-scenarios

    // system default: display all errors
    SetErrorMode(0);

    // when the application crashes due to an exception, call this handler
    SetUnhandledExceptionFilter(crash_handler);

    signal(SIGABRT, signal_handler);
}

void osc::OpenPathInOSDefaultApplication(std::filesystem::path const& p)
{
    ShellExecute(0, 0, p.string().c_str(), 0, 0 , SW_SHOW );
}

void osc::OpenURLInDefaultBrowser(std::string_view)
{
    log::error("unsupported action: cannot open external URLs in Windows (yet!)");
}
#else
#error "Unsupported platform?"
#endif
