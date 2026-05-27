#include "config.h"

#include <libopynsim/opynsim.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message_view.h>
#include <liboscar/platform/log_sink.h>
#include <liboscar/utilities/enum_helpers.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>  // care: used by casters (might be incorrectly flagged as unused)
#include <nanobind/stl/string.h>      // care: used by casters (might be incorrectly flagged as unused)
#include <nanobind/stl/vector.h>      // care: used by casters (might be incorrectly flagged as unused)

#include <memory>
#include <mutex>
#include <utility>

namespace nb = nanobind;

namespace
{
    osc::LogLevel to_oscar_log_level(int python_logging_level)
    {
        static_assert(osc::num_options<osc::LogLevel>() == 7, "review this code if oscar levels change");

        // These are dictated by the python documentation: https://docs.python.org/3/library/logging.html#logging-levels
        switch (python_logging_level) {
            case 10: return osc::LogLevel::debug;     // logging.DEBUG
            case 20: return osc::LogLevel::info;      // logging.INFO
            case 30: return osc::LogLevel::warn;      // logging.WARNING
            case 40: return osc::LogLevel::err;       // logging.ERROR
            case 50: return osc::LogLevel::critical;  // logging.CRITICAL
            case 0 : [[fallthrough]];                 // logging.NOTSET
            default: return osc::LogLevel::DEFAULT;
        }
    }

    int to_python_log_level(osc::LogLevel oscar_logging_level)
    {
        static_assert(osc::num_options<osc::LogLevel>() == 7, "review this code if oscar levels change");

        // These are dictated by the python documentation: https://docs.python.org/3/library/logging.html#logging-levels
        switch (oscar_logging_level) {
            case osc::LogLevel::trace:    [[fallthrough]];
            case osc::LogLevel::debug:    return 10;        // logging.DEBUG
            case osc::LogLevel::info:     return 20;        // logging.INFO
            case osc::LogLevel::warn:     return 30;        // logging.WARNING
            case osc::LogLevel::err:      return 40;        // logging.ERROR
            case osc::LogLevel::critical: return 50;        // logging.CRITICAL
            default:                      return 20;        // logging.INFO
        }
    }

    class PythonLoggingSink : public osc::LogSink {
    public:
        explicit PythonLoggingSink()
        {
            set_level(osc::LogLevel::warn);  // Python's default

            // Pre-cache the lookup for OPynSim's logger object
            nb::gil_scoped_acquire gil_lock;
            python_logger_ = nb::module_::import_("logging").attr("getLogger")("opynsim");
            is_enabled_for_method_ = python_logger_.attr("isEnabledFor");
            log_method_ = python_logger_.attr("log");
        }
        PythonLoggingSink(const PythonLoggingSink&) = delete;
        PythonLoggingSink(PythonLoggingSink&&) noexcept = delete;
        ~PythonLoggingSink() noexcept override
        {
            nb::gil_scoped_acquire gil_lock;
            log_method_.release();
            is_enabled_for_method_.release();
            python_logger_.release();
        }

        PythonLoggingSink& operator=(const PythonLoggingSink&) = delete;
        PythonLoggingSink& operator=(PythonLoggingSink&&) noexcept = delete;

        void impl_sink_message(const osc::LogMessageView& log_message) override
        {
            // This sink is only called if the C++ logger is set to the appropriate
            // level by a user calling `opynsim.config.set_log_level`...

            const int py_message_level = to_python_log_level(log_message.level());

            nb::gil_scoped_acquire gil_lock;
            const std::lock_guard log_lock{log_mutex_};

            // ... but the implementation should also check the current state of the Python
            //     logging system, which may hierarchically filter messages with a root logger.
            if (nb::cast<bool>(is_enabled_for_method_(py_message_level))) {
                log_method_(py_message_level, std::string{log_message.payload()});
            }
        }
    private:
        std::mutex log_mutex_;      // Protects `nb::object` accesses when Python is free-threaded (GIL-less)
        nb::object python_logger_;  // Keeps the logger alive as long as the methods (below)
        nb::object is_enabled_for_method_;
        nb::object log_method_;
    };

    void setup_python_based_logging()
    {
        // Initialize the `opynsim` C++ log with the same default logging level
        // as Python (warning).
        opyn::set_log_level(osc::LogLevel::warn);

        // Create an `oscar` (C++) log sink that sink its messages via the Python
        // `logging` API, so that Python developers can handle the messages after
        // they come through the pipe (e.g. so that Python code can separately
        // filter via a root logger, or designate a logging file.
        auto sink = std::make_shared<PythonLoggingSink>();

        // Make the Python log sink the only (and default)sink of the top-level
        // default logger.
        auto global_logger = osc::global_default_logger();
        global_logger->sinks().clear();
        global_logger->sinks().push_back(std::move(sink));

        // Ensure the log sink is destroyed when Python exits, rather than when
        // oscar's static destructor runs, because the static destruction might
        // happen after Python is already dead.
        nb::module_::import_("atexit").attr("register")(nb::cpp_function([]() { osc::global_default_logger()->sinks().clear(); }));
    }
}

void opyn::init_config_submodule(nanobind::module_& config_module)
{
    // Before doing anything, ensure the C++ log is directly integrated
    // with the Python logging system, so that Python scripts with custom
    // log levels etc. can immediately see any initialization errors.
    setup_python_based_logging();

    // Globally initialize the opynsim API (load all components, etc.)
    opyn::init();

    config_module.def(
        "get_log_level",
        []{ return to_python_log_level(get_log_level()); },
        R"(
            Returns the current global logging level for OPynSim. The default log level is ``logging.WARNING``.

            If the logger has been misconfigured (e.g. because the log level was edited via Python, rather than
            via :func:`opynsim.set_log_level`) then the value returned by this function may not match
            ``logger.getLogger("opynsim").level``. See :func:`set_log_level` for more details.

            Returns:
                int: A logging level integer that's compatible with the Python (:mod:`logging`) module such as ``logging.DEBUG``, ``logging.INFO``, ``logging.WARNING``, ``logging.ERROR``, or ``logging.CRITICAL`` see `Python's Logging Levels documentation <https://docs.python.org/3/library/logging.html#logging-levels>`_ for more information on the logging levels.
        )"
    );

    config_module.def(
        "set_log_level",
        [](int python_logging_level)
        {
            // Set the C++ logging level.
            set_log_level(to_oscar_log_level(python_logging_level));

            // Additionally, set the Python logging level (the user might forget to sync them).
            nb::module_::import_("logging").attr("getLogger")("opynsim").attr("setLevel")(python_logging_level);
        },
        nb::arg("python_logging_level"),
        R"(
            Sets the global logging level for OPynSim. The default log level is ``logging.WARN``.

            ``opynsim`` is integrated with `Python's logging API <https://docs.python.org/3/library/logging.html>`_
            but, for performance reasons, its C++ engine stores an internal log level separately. This function
            synchronizes the log level of both OPynSim's Python logger (i.e. ``logger.getLogger("opynsim")``) and
            the internal C++ log level. Changing only the Python logger can result in a situation where the C++
            engine emits (or doesn't!) log messages that Python logger subsequently drops.

            Args:
                python_logging_level (int): A logging level from the Python (:mod:`logging`) module
                    such as ``logging.DEBUG``, ``logging.INFO``, ``logging.WARNING``, ``logging.ERROR``,
                    or ``logging.CRITICAL`` see `Python's Logging Levels documentation <https://docs.python.org/3/library/logging.html#logging-levels>`_ for
                    more information on the logging levels.
        )"
    );

    config_module.def(
        "get_search_paths",
        &opyn::get_search_paths,
        R"(
            Returns a copy of the current global search paths used to resolve filesystem resources,
            ordered from highest to lowest priority.

            When OPynSim resolves a relative ``path``, it uses a context-dependent ``base_path``
            (e.g., the directory containing the current model file) and probes locations in this order:

            1. If ``path`` is absolute: Probes only ``path``
            2. If ``path`` is relative: For each ``entry`` in ``get_search_path()``, probe ``(base_path / entry / path)``.

            Notably, if an ``entry`` is absolute, ``(base_path / entry / path) == (entry / path)``, so
            the ``base_path`` is ignored for that entry.

            When a resource cannot be found via probing, the consequences are context-dependent. For example, a
            mesh implementation that fails to find a mesh file may choose to ignore the failure, generate
            a debug/error mesh, or throw an exception.

            The returned paths will be lexicographically unique, but entries may not be be behaviorally
            unique. For example, ``pathlib.Path("Geometry") == pathlib.Path("geometry")`` is ``True`` on
            Windows but ``False`` on macOS and Linux. Regardless, the OPynSim API for search paths checks
            and compares the lexicographic representation.
        )"
    );

    config_module.def(
        "prepend_search_path",
        &opyn::prepend_search_path,
        nb::arg("search_path"),
        R"(
            Prepends ``search_path`` to the start (highest-priority) of the global
            search path list (see :func:`get_search_paths`).

            If an existing entry in the global search path list lexicographically
            matches ``search_path``, it is removed to prevent duplicate entries.
            ``search_path`` does not need to exist on the filesystem.
        )"
    );

    config_module.def(
        "append_search_path",
        &opyn::append_search_path,
        nb::arg("search_path"),
        R"(
            Appends ``search_path`` to the end (lowest-priority) of the global
            search path list (see :func:`get_search_paths`).

            If an existing entry in the global search path list lexicographically
            matches ``search_path``, it is removed to prevent duplicate entries.
            ``search_path`` does not need to exist on the filesystem.
        )"
    );

    config_module.def(
        "remove_search_path",
        &opyn::remove_search_path,
        nb::arg("search_path"),
        R"(
            Returns ``True`` if an entry that lexicographically matches ``search_path`` was
            found and removed from the global search path list.

            Args:
                search_path: The path to remove. It must lexicographically match an entry from :func:`get_search_paths`.
        )"
    );
}
