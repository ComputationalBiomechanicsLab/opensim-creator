#include "os.hpp"

#include "src/Assertions.hpp"
#include "src/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <nfd.h>
#include <SDL_clipboard.h>
#include <SDL_error.h>
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>

#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using std::literals::string_literals::operator""s;

static std::filesystem::path convertSDLPathToStdpath(char const* methodname, char* p)
{
    if (p == nullptr)
    {
        std::stringstream ss;
        ss << methodname << ": returned null: " << SDL_GetError();
        throw std::runtime_error{std::move(ss).str()};
    }

    size_t len = std::strlen(p);

    if (len == 0)
    {
        std::stringstream ss;
        ss << methodname << ": returned an empty string";
        throw std::runtime_error{std::move(ss).str()};
    }

    // remove trailing slash: it interferes with std::filesystem::path
    p[len - 1] = '\0';

    return std::filesystem::path{p};
}

static std::filesystem::path getCurrentExeDir()
{
    std::unique_ptr<char, decltype(&SDL_free)> p{SDL_GetBasePath(), SDL_free};

    return convertSDLPathToStdpath("SDL_GetBasePath", p.get());
}

static std::filesystem::path getUserDataDir()
{
    std::unique_ptr<char, decltype(&SDL_free)> p{SDL_GetPrefPath("cbl", "osc"), SDL_free};

    return convertSDLPathToStdpath("SDL_GetPrefPath", p.get());
}

std::filesystem::path const& osc::CurrentExeDir()
{
    // can be expensive to compute: cache after first retrieval
    static std::filesystem::path const d = getCurrentExeDir();

    return d;
}

std::filesystem::path const& osc::GetUserDataDir()
{
    // can be expensive to compute: cache after first retrieval
    static std::filesystem::path const d = getUserDataDir();

    return d;
}

bool osc::SetClipboardText(char const* s)
{
    return SDL_SetClipboardText(s) == 0;
}

void osc::SetEnv(char const* name, char const* value, bool overwrite)
{
    SDL_setenv(name, value, overwrite ? 1 : 0);
}

std::filesystem::path osc::PromptUserForFile(char const* extensions, char const* defaultPath)
{
    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_OpenDialog(extensions, defaultPath, &path);
    OSC_SCOPE_GUARD_IF(path, { free(path); });

    return (path && result == NFD_OKAY) ? std::filesystem::path{path} : std::filesystem::path{};
}

std::vector<std::filesystem::path> osc::PromptUserForFiles(char const* extensions, char const* defaultPath)
{
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple(extensions, defaultPath, &s);

    std::vector<std::filesystem::path> rv;

    if (result == NFD_OKAY)
    {
        size_t len = NFD_PathSet_GetCount(&s);
        rv.reserve(len);
        for (size_t i = 0; i < len; ++i)
        {
            rv.push_back(NFD_PathSet_GetPath(&s, i));
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

std::filesystem::path osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary(char const* extension, char const* defaultPath)
{
    if (extension)
    {
        OSC_ASSERT(!Contains(extension, ',') && "can only provide one extension to this implementation!");
    }

    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_SaveDialog(extension, defaultPath, &path);
    OSC_SCOPE_GUARD_IF(path, { free(path); });

    if (result != NFD_OKAY)
    {
        return std::filesystem::path{};
    }

    std::filesystem::path p{path};

    if (extension && !CStrEndsWith(path, extension))
    {
        p += '.';
        p += extension;
    }

    return p;
}

#ifdef __LINUX__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void osc::WriteTracebackToLog(log::level::LevelEnum lvl)
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

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext {
    unsigned long uc_flags;
    ucontext_t* uc_link;
    stack_t uc_stack;
    sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

static void OnCriticalSignalRecv(int sig_num, siginfo_t* info, void* ucontext)
{
    sig_ucontext_t* uc = static_cast<sig_ucontext_t*>(ucontext);

    /* Get the address at the time the signal was raised */
#if defined(__i386__)  // gcc specific
    void* caller_address = reinterpret_cast<void*>(uc->uc_mcontext.eip);  // EIP: x86 specific
#elif defined(__x86_64__)  // gcc specific
    void* callerAddress = reinterpret_cast<void*>(uc->uc_mcontext.rip);  // RIP: x86_64 specific
#else
#error Unsupported architecture.
#endif

    fprintf(
        stderr,
        "osc: critical error: signal %d (%s) received from OS: address is %p from %p\n",
        sig_num,
        strsignal(sig_num),
        info->si_addr,
        callerAddress);

    void* array[50];
    int size = backtrace(array, 50);
    array[1] = callerAddress;  // overwrite sigaction with caller's address

    char** messages = backtrace_symbols(array, size);

    if (messages == nullptr)
    {
        exit(EXIT_FAILURE);
    }

    OSC_SCOPE_GUARD({ free(messages); });

    /* skip first stack frame (points here) */
    for (int i = 1; i < size; ++i)
    {
        fprintf(stderr, "    #%-2d %s\n", i, messages[i]);
    }

    exit(EXIT_FAILURE);
}

void osc::InstallBacktraceHandler()
{
    struct sigaction sigact;

    sigact.sa_sigaction = OnCriticalSignalRecv;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    // install segfault handler
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        fprintf(
            stderr,
            "osc: warning: could not set signal handler for %d (%s): error reporting may not work as intended\n",
            SIGSEGV,
            strsignal(SIGSEGV));
    }

    // install abort handler: this triggers whenever a non-throwing `assert` causes a termination
    if (sigaction(SIGABRT, &sigact, nullptr) != 0)
    {
        fprintf(
            stderr,
            "osc: warning: could not set signal handler for %d (%s): error reporting may not work as intended\n",
            SIGABRT,
            strsignal(SIGABRT));
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

        int rv;
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
        int rv = execlp("xdg-open", "xdg-open", fp.c_str(), (char*)NULL);

        // this thread only reaches here if there is some kind of error in `exec`
        //
        // aggressively exit this thread, returning the status code. Do **not**
        // return from this thread, because it should'nt behave as-if it were
        // the calling thread
        exit(rv);
    }
}

void osc::OpenURLInDefaultBrowser(std::string_view vw)
{
    // HACK: we know that xdg-open handles this automatically
    OpenPathInOSDefaultApplication(vw);
}

#elif defined(__APPLE__)
#include <execinfo.h>  // backtrace(), backtrace_symbols()
#include <signal.h>  // sigaction(), struct sigaction, strsignal()
#include <stdlib.h>  // exit(), free()

void osc::WriteTracebackToLog(log::level::LevelEnum lvl)
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

[[noreturn]] static void OSC_critical_error_handler(int sig_num, siginfo_t* info, void* ucontext)
{
    osc::log::error("critical error: signal %d (%s) received from OS", sig_num, strsignal(sig_num));
    osc::log::error("backtrace:");
    osc::WriteTracebackToLog(osc::log::level::err);
    exit(EXIT_FAILURE);
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
#include <cinttypes>  // PRIXPTR
#include <signal.h>   // signal()

void osc::WriteTracebackToLog(log::level::LevelEnum lvl)
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

static LONG crash_handler(EXCEPTION_POINTERS* info)
{
    osc::log::error("exception propagated to root of OSC: might be a segfault?");
    osc::WriteTracebackToLog(osc::log::level::err);
    return EXCEPTION_CONTINUE_SEARCH;
}

static void signal_handler(int signal)
{
    osc::log::error("signal caught by OSC: printing backtrace");
    osc::WriteTracebackToLog(osc::log::level::err);
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
