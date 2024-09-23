#include "os.h"

#include <oscar/Platform/Log.h>
#include <oscar/Platform/LogLevel.h>
#include <oscar/Platform/LogSink.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/StringHelpers.h>

#ifndef EMSCRIPTEN
    #include <nfd.h>
#endif
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
#include <random>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path convert_SDL_filepath_to_std_filepath(CStringView methodname, char* p)
    {
        // nullptr disallowed
        if (p == nullptr) {
            std::stringstream ss;
            ss << methodname << ": returned null: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }

        std::string_view sv{p};

        // empty string disallowed
        if (sv.empty()) {
            std::stringstream ss;
            ss << methodname << ": returned an empty string";
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
    const std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    return gmtime_threadsafe(t);
}

std::filesystem::path osc::current_executable_directory()
{
    std::unique_ptr<char, decltype(&SDL_free)> p{
        SDL_GetBasePath(),
        SDL_free
    };
    return convert_SDL_filepath_to_std_filepath("SDL_GetBasePath", p.get());
}

std::filesystem::path osc::user_data_directory(
    CStringView organization_name,
    CStringView application_name)
{
    std::unique_ptr<char, decltype(&SDL_free)> p{
        SDL_GetPrefPath(organization_name.c_str(), application_name.c_str()),
        SDL_free,
    };
    return convert_SDL_filepath_to_std_filepath("SDL_GetPrefPath", p.get());
}

std::string osc::get_clipboard_text()
{
    char* str = SDL_GetClipboardText();
    if (str) {
        ScopeGuard guard{[str]() { SDL_free(str); }};
        return std::string(str);
    }
    else {
        return std::string{};
    }
}

bool osc::set_clipboard_text(CStringView content)
{
    return SDL_SetClipboardText(content.c_str()) == 0;
}

void osc::set_environment_variable(CStringView name, CStringView value, bool overwrite)
{
    SDL_setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0);
}

std::optional<std::filesystem::path> osc::prompt_user_to_select_file(
    std::span<const std::string_view> file_extensions,
    std::optional<std::filesystem::path> initial_directory_to_show)
{
#ifdef EMSCRIPTEN
    static_cast<void>(file_extensions);
    static_cast<void>(initial_directory_to_show);
    return std::nullopt;
#else

    auto [path, result] = [&]()
    {
        const std::string comma_delimted_extensions = join(file_extensions, ",");

        nfdchar_t* ptr = nullptr;
        const nfdresult_t res = NFD_OpenDialog(
            comma_delimted_extensions.empty() ? nullptr : comma_delimted_extensions.c_str(),
            initial_directory_to_show ? initial_directory_to_show->string().c_str() : nullptr,
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
#endif
}

std::vector<std::filesystem::path> osc::prompt_user_to_select_files(
    std::span<const std::string_view> file_extensions,
    std::optional<std::filesystem::path> initial_directory_to_show)
{
#ifdef EMSCRIPTEN
    static_cast<void>(file_extensions);
    static_cast<void>(initial_directory_to_show);
    return {};
#else

    const std::string comma_delimted_extensions = join(file_extensions, ",");
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple(
        comma_delimted_extensions.empty() ? nullptr : comma_delimted_extensions.c_str(),
        initial_directory_to_show ? initial_directory_to_show->string().c_str() : nullptr,
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
#endif
}

std::optional<std::filesystem::path> osc::prompt_user_for_file_save_location_add_extension_if_necessary(
    std::optional<CStringView> maybe_extension,
    std::optional<CStringView> maybe_initial_directory_to_open)
{
#ifdef EMSCRIPTEN
    static_cast<void>(maybe_extension);
    static_cast<void>(maybe_initial_directory_to_open);
    return std::nullopt;
#else
    if (maybe_extension)
    {
        OSC_ASSERT(!contains(*maybe_extension, ',') && "can only provide one extension to this implementation!");
    }

    auto [path, result] = [&]()
    {
        nfdchar_t* ptr = nullptr;
        const nfdresult_t res = NFD_SaveDialog(
            maybe_extension ? maybe_extension->c_str() : nullptr,
            maybe_initial_directory_to_open ? maybe_initial_directory_to_open->c_str() : nullptr,
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

    if (maybe_extension)
    {
        // ensure that the user-selected path is tested against '.$EXTENSION' (#771)
        //
        // the caller only provides the extension without the dot (this is what
        // NFD requires) but the user may have manually wrote a string that is
        // suffixed with the dot-less version of the extension (e.g. "somecsv")

        const std::string fullExtension = std::string{"."} + *maybe_extension;
        if (!std::string_view{path.get()}.ends_with(fullExtension)) {
            p += fullExtension;
        }
    }

    return p;
#endif
}

std::string osc::errno_to_string_threadsafe()
{
    return strerror_threadsafe(errno);
}

namespace
{
    const std::string_view c_valid_dynamic_characters = "abcdefghijklmnopqrstuvwxyz0123456789";
    void write_dynamic_name_els(std::ostream& out)
    {
        static std::default_random_engine s_prng{std::random_device{}()};
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
        if (auto fd = std::fopen(attempt_path.string().c_str(), "w+x"); fd != nullptr) {  // TODO: replace with `std::ios_base::noreplace` in C++23
            std::fclose(fd);
            return {std::fstream{attempt_path, std::ios_base::in | std::ios_base::out | std::ios_base::binary}, std::move(attempt_path)};
        }
#pragma warning(pop)
    }
    throw std::runtime_error{"failed to create a unique temporary filename after 100 attempts - are you creating _a lot_ of temporary files? ;)"};
}


#ifdef __LINUX__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <execinfo.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using osc::log_error;

std::tm osc::gmtime_threadsafe(std::time_t t)
{
    std::tm rv{};
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::strerror_threadsafe(int errnum)
{
    std::array<char, 1024> buffer{};

    auto* maybeErr = strerror_r(errnum, buffer.data(), buffer.size());
    if (std::is_same_v<int, decltype(maybeErr)> && !maybeErr)
    {
        log_warn("a call to strerror_r failed with error code %i", maybeErr);
        return {};
    }
    else
    {
        static_cast<void>(maybeErr);
    }

    std::string rv{buffer.data()};
    if (rv.size() == buffer.size())
    {
        log_warn("a call to strerror_r returned an error string that was as big as the buffer: an OS error message may have been truncated!");
    }
    return rv;
}

void osc::write_this_thread_backtrace_to_log(LogLevel lvl)
{
    std::array<void*, 50> ary{};
    const int size = backtrace(ary.data(), ary.size());
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
#ifdef EMSCRIPTEN
        static_cast<void>(sig_num);
        static_cast<void>(info);
        static_cast<void>(ucontext);
#else
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
        const int size = backtrace(ary.data(), ary.size());
        ary[1] = callerAddress;  // overwrite sigaction with caller's address

        const std::unique_ptr<char*, decltype(::free)*> messages{backtrace_symbols(ary.data(), size), ::free};
        if (!messages)
        {
            return;
        }

        /* skip first stack frame (points here) */
        for (int i = 1; i < size; ++i)
        {
            std::cerr << "    #" << std::setw(2) << i << ' ' << messages.get()[i] << '\n';
        }
#endif
    }
}

void osc::enable_crash_signal_backtrace_handler(const std::filesystem::path&)
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

void osc::open_file_in_os_default_application(const std::filesystem::path& fp)
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

void osc::open_url_in_os_default_web_browser(CStringView vw)
{
    // (we know that xdg-open handles this automatically)
    open_file_in_os_default_application(std::filesystem::path{std::string{vw}});
}

void osc::enable_highdpi_mode_for_this_process() {}

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

std::tm osc::gmtime_threadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_r(&t, &rv);
    return rv;
}

std::string osc::strerror_threadsafe(int errnum)
{
    std::array<char, 512> buffer{};
    if (strerror_r(errnum, buffer.data(), buffer.size()) == ERANGE)
    {
        log_warn("a call to strerror_r returned ERANGE: an OS error message may have been truncated!");
    }
    return std::string{buffer.data()};
}


void osc::write_this_thread_backtrace_to_log(LogLevel lvl)
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
        osc::write_this_thread_backtrace_to_log(osc::LogLevel::err);
        exit(EXIT_FAILURE);
    }
}

void osc::enable_crash_signal_backtrace_handler(const std::filesystem::path&)
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

void osc::open_file_in_os_default_application(const std::filesystem::path& p)
{
    std::string cmd = "open " + p.string();
    system(cmd.c_str());
}

void osc::open_url_in_os_default_web_browser(CStringView url)
{
    std::string cmd = "open " + std::string{url};
    system(cmd.c_str());
}

void osc::enable_highdpi_mode_for_this_process() {}

#elif defined(WIN32)
#include <Windows.h>  // PVOID, RtlCaptureStackBackTrace(), MEMORY_BASIC_INFORMATION, VirtualQuery(), DWORD64, TCHAR, GetModuleFileName()
#include <time.h>  // gmtime_s
#include <cinttypes>  // PRIXPTR
#include <signal.h>   // signal()

#include <oscar/Shims/Cpp20/bit.h>

using osc::global_default_logger;
using osc::global_get_traceback_log;
using osc::LogMessage;
using osc::LogMessageView;
using osc::LogSink;
using osc::to_cstringview;

std::tm osc::gmtime_threadsafe(std::time_t t)
{
    std::tm rv;
    gmtime_s(&rv, &t);
    return rv;
}

std::string osc::strerror_threadsafe(int errnum)
{
    std::array<char, 512> buf{};
    if (errno_t rv = strerror_s(buf.data(), buf.size(), errnum); rv != 0) {
        log_warn("a call to strerror_s returned an error (%i): an OS error message may be missing!", rv);
    }
    return std::string{buf.data()};
}

void osc::write_this_thread_backtrace_to_log(LogLevel lvl)
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
        {}

    private:
        void impl_sink_message(const LogMessageView& msg) final
        {
            if (m_Out) {
                m_Out << '[' << msg.logger_name() << "] [" << msg.level() << "] " << msg.payload() << std::endl;
            }
        }

        std::ostream& m_Out;
    };

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds get_current_time_as_unix_timestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    // care: this is necessary because segfault crash handlers don't appear to
    // be able to have data passed to them
    std::optional<std::filesystem::path> upd_crash_report_directory_global()
    {
        static std::optional<std::filesystem::path> s_CrashDumpDir;
        return s_CrashDumpDir;
    }

    std::optional<std::filesystem::path> get_crash_report_path()
    {
        if (!upd_crash_report_directory_global()) {
            return std::nullopt;  // global wasn't set: programmer error
        }

        std::stringstream filename;
        filename << get_current_time_as_unix_timestamp().count();
        filename << "_CrashReport.txt";
        return *upd_crash_report_directory_global() / std::move(filename).str();
    }

    LONG crash_handler(EXCEPTION_POINTERS*)
    {
        log_error("exception propagated to root of the application: might be a segfault?");

        const std::optional<std::filesystem::path> maybe_crash_report_path =
            get_crash_report_path();

        std::optional<std::ofstream> maybe_crash_report_ostream = maybe_crash_report_path ?
            std::ofstream{*maybe_crash_report_path} :
            std::optional<std::ofstream>{};

        // dump out the log history (it's handy for context)
        if (maybe_crash_report_ostream and *maybe_crash_report_ostream)
        {
            *maybe_crash_report_ostream << "----- log -----\n";
            auto guard = global_get_traceback_log().lock();
            for (const LogMessage& msg : *guard) {
                *maybe_crash_report_ostream << '[' << msg.logger_name() << "] [" << msg.level() << "] " << msg.payload() << '\n';
            }
            *maybe_crash_report_ostream << "----- /log -----\n";
        }

        // then write a traceback to both the log (in case the user is running from a console)
        // *and* the crash dump (in case the user is running from a GUI and wants to report it)
        if (maybe_crash_report_ostream and *maybe_crash_report_ostream) {
            *maybe_crash_report_ostream << "----- traceback -----\n";

            auto sink = std::make_shared<CrashFileSink>(*maybe_crash_report_ostream);

            global_default_logger()->sinks().push_back(sink);
            write_this_thread_backtrace_to_log(osc::LogLevel::err);
            global_default_logger()->sinks().erase(global_default_logger()->sinks().end() - 1);

            *maybe_crash_report_ostream << "----- /traceback -----\n";
        }
        else {
            // (no crash dump file, but still write it to stdout etc.)

            *maybe_crash_report_ostream << "----- traceback -----\n";
            write_this_thread_backtrace_to_log(osc::LogLevel::err);
            *maybe_crash_report_ostream << "----- /traceback -----\n";
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void signal_handler(int)
    {
        log_error("signal caught by application: printing backtrace");
        write_this_thread_backtrace_to_log(osc::LogLevel::err);
    }
}

void osc::enable_crash_signal_backtrace_handler(const std::filesystem::path& crash_dump_directory)
{
    // https://stackoverflow.com/questions/13591334/what-actions-do-i-need-to-take-to-get-a-crash-dump-in-all-error-scenarios

    // set crash dump directory globally so that the crash handler can see it
    upd_crash_report_directory_global() = crash_dump_directory;

    // system default: display all errors
    SetErrorMode(0);

    // when the application crashes due to an exception, call this handler
    SetUnhandledExceptionFilter(crash_handler);

    signal(SIGABRT, signal_handler);
}

void osc::open_file_in_os_default_application(const std::filesystem::path& p)
{
    ShellExecute(0, 0, p.string().c_str(), 0, 0 , SW_SHOW);
}

void osc::open_url_in_os_default_web_browser(CStringView url)
{
    ShellExecute(0, 0, url.c_str(), 0, 0 , SW_SHOW);
}

void osc::enable_highdpi_mode_for_this_process()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
}

#elif EMSCRIPTEN

void osc::enable_crash_signal_backtrace_handler(const std::filesystem::path&) {}

std::tm osc::gmtime_threadsafe(std::time_t t)
{
    std::tm rv{};
    gmtime_r(&t, &rv);
    return rv;
}
void osc::enable_highdpi_mode_for_this_process() {}

#else
#error "Unsupported platform?"
#endif
