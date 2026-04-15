#include "core.h"

#include <opynsim/_core/graphics.h>
#include <opynsim/_core/tps3d.h>
#include <opynsim/_core/ui.h>

#include <liboscar/platform/log.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message_view.h>
#include <liboscar/platform/log_sink.h>
#include <liboscar/utilities/enum_helpers.h>
#include <libopynsim/platform/opynsim_app.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>
#include <libopynsim/symbol.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace opyn;

namespace nb = nanobind;

namespace
{
    std::unique_ptr<OPynSimApp> g_lazy_loaded_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
            case osc::LogLevel::trace:    return 10;  // logging.DEBUG
            case osc::LogLevel::debug:    return 10;  // logging.DEBUG
            case osc::LogLevel::info:     return 20;  // logging.INFO
            case osc::LogLevel::warn:     return 30;  // logging.WARNING
            case osc::LogLevel::err:      return 40;  // logging.ERROR
            case osc::LogLevel::critical: return 50;  // logging.CRITICAL
            default:                      return 20;  // logging.INFO
        }
    }

    template<typename CppType>
    struct PythonTypeMapper {
        using cpp_type = CppType;
        using python_type = CppType;

        static python_type to_python(cpp_type&& v) { return python_type{std::move(v)}; }
    };

    template<>
    struct PythonTypeMapper<osc::Vector3d> {
        using cpp_type = osc::Vector3d;
        using python_type = nb::ndarray<nb::numpy, double, nb::shape<3>>;

        static python_type to_python(cpp_type&& v) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        {
            auto* data = new double[3]{v[0], v[1], v[2]};  // NOLINT(cppcoreguidelines-owning-memory)
            const std::array<size_t, 1> shape = {3};
            return {
                data,
                1,
                shape.data(),
                nb::capsule(data, [](void* p) noexcept { delete[] static_cast<double*>(p); })  // NOLINT(cppcoreguidelines-owning-memory)
            };
        }
    };

    template<typename... Ts>
    auto to_python_mapper_typelist(osc::Typelist<Ts...>)
    {
        return osc::Typelist<typename PythonTypeMapper<Ts>::python_type...>{};
    }

    using SupportedPythonOutputValues = decltype(to_python_mapper_typelist(std::declval<SupportedOutputValueTypes>()));
    using PythonOutputValue = osc::VariantOfTypelistElements<SupportedPythonOutputValues>;

    PythonOutputValue to_python_output(OutputValue&& output_value)
    {
        return std::visit(osc::Overload{
            []<typename T>(T&& v) -> PythonOutputValue { return PythonTypeMapper<T>::to_python(std::forward<T>(v)); }
        }, std::move(output_value));  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
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
            // level by a user calling `opynsim.set_log_level`...

            const int py_message_level = to_python_log_level(log_message.level());

            nb::gil_scoped_acquire gil_lock;
            std::lock_guard log_lock{log_mutex_};

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

opyn::OPynSimApp& opyn::get_lazy_loaded_opynsim_app()
{
    if (not g_lazy_loaded_app) {
        g_lazy_loaded_app = std::make_unique<OPynSimApp>();
    }
    return *g_lazy_loaded_app;
}

void opyn::destroy_lazy_loaded_opynsim_app()
{
    g_lazy_loaded_app.reset();
}

NB_MODULE(_core, _core_module)  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
{
    // Before doing anything, ensure the C++ log is directly integrated
    // with the Python logging system, so that Python scripts with custom
    // log levels etc. can immediately see any initialization errors.
    setup_python_based_logging();

    // Install an exit handler that cleans up any lazy-loaded application state
    // when the Python interpreter shuts down
    nb::module_::import_("atexit").attr("register")(nb::cpp_function(&destroy_lazy_loaded_opynsim_app));

    // Globally initialize the opynsim API (load all components, etc.)
    opyn::init();

    // Initialize `graphics` submodule.
    {
        auto graphics_submodule = _core_module.def_submodule("graphics");
        init_graphics_submodule(graphics_submodule);
    }

    // Initialize `tps3d` submodule.
    {
        auto tps3d_submodule = _core_module.def_submodule("tps3d");
        init_tps3d_submodule(tps3d_submodule);
    }

    // Initialize `ui` submodule.
    {
        auto ui_submodule = _core_module.def_submodule("ui");
        init_ui_submodule(ui_submodule);
    }

    // Initialize top-level functions/classes
    {
        nb::class_<Symbol> symbol_class(
            _core_module,
            "Symbol",
            R"(
                Represents an immutable, cheap-to-use, readable symbol.

                Symbols are extensively used by the OPynSim API to accelerate associative lookups. They are the
                middle-ground between fast, but hard to read/introspect, integer handles and slow, simpler string
                handles.

                From Python code's point of view, symbols should be seen as string-like handles that OPynSim
                accepts/emits. You can safely store symbols independently of any larger data structure, and
                interchange them across your entire Python codebase, without having to worry about object
                lifetimes. OPynSim's native code uses runtime-checked associative lookups, rather than pointers, to
                ensure that the Python API is memory-safe and can provide suitable feedback whenever a lookup fails.
            )"
        );
        symbol_class.def(
            nb::init<std::string_view>(),
            nb::arg("id"),
            R"(
                Constructs a symbol from a Python string.
            )"
        );
        symbol_class.def(
            "__str__",
            [](const Symbol& symbol) { return static_cast<std::string>(symbol); },
            "Converts this symbol into a Python :class:`str`"
        );
        symbol_class.def("__repr__", [](const Symbol& self) { return std::string("Symbol(\"") + std::string(self) + "\")"; });
        symbol_class.def("__hash__", [](const Symbol& self) { return std::hash<Symbol>{}(self); });
        symbol_class.def("__eq__",   [](const Symbol& self, const Symbol& other)  { return self == other; });
        symbol_class.def("__eq__",   [](const Symbol& self, std::string_view rhs) { return self == rhs; });
        symbol_class.def("__contains__", [](const Symbol& self, std::string_view rhs) { return static_cast<std::string_view>(self).find(rhs) != std::string_view::npos; });
        nb::implicitly_convertible<std::string_view, Symbol>();

        nb::class_<ModelSpecification> model_specification_class(
            _core_module,
            "ModelSpecification",
            R"(
                A high-level specification for a :class:`Model`.

                A :class:`ModelSpecification` is what Python code can manipulate, scale, and customize
                before passing it to :meth:`compile`, which returns a readonly :class:`Model`.

                Notes:
                    OPynSim's API design separates the specification of a model (:class:`ModelSpecification`)
                    from its validated, assembled, and optimized simulation representation (:class:`Model`) to ensure
                    that the compilation process (:meth:`compile`) can freeze and optimize internal
                    datastructures at a single point in the process.
            )"
        );
        model_specification_class.def(nb::init<>());  // Define default constructor
        model_specification_class.def(
            "compile",
            &ModelSpecification::compile,
            R"(
                Compiles this :class:`ModelSpecification` into a :class:`Model`.

                The compilation process:

                - Validates this :class:`ModelSpecification`'s components (properties, subcomponents,
                  and sockets), throwing an exception if the specification is invalid in some way.
                - Assembles a physics system from the validated specification, throwing an exception
                  if the physics system cannot be assembled (e.g. if the contains impossible-to-satisfy
                  joints, or invalid muscle definitions).

                Raises:
                    Exception: If the compilation process failed in some way. It is assumed that
                        the provided :class:`ModelSpecification` is valid.
            )"
        );

        nb::class_<Model> model_class(
            _core_module,
            "Model",
            R"(
                A validated, optimized, compiled, and ready-to-simulate model of a physics system.

                A :class:`Model` can only be created from a :class:`ModelSpecification` via the
                :meth:`ModelSpecification.compile` function. Therefore, editing a :class:`Model` requires
                editing its associated :class:`ModelSpecification` and recompiling it to create a
                new :class:`Model`.
            )"
        );
        model_class.def_prop_ro(
            "num_coordinates",
            &Model::num_coordinates,
            R"(
                Returns the number of coordinates in the model.

                A coordinate represents a single degree of freedom (DoF) in the model, such as a joint angle,
                translation, or rotational parameter that contributes to the configuration/pose of a model.
            )"
        );
        model_class.def_prop_ro(
            "coordinates",
            &Model::coordinates,
            R"(
                Returns a list of all the coordinates in the model.
            )"
        );
        model_class.def(
            "initial_state",
            &Model::initial_state,
            R"(
                Returns a :class:`ModelState` that represents the initial (default) state of this :class:`Model`.

                The initial state of a :class:`Model` is dictated by its associated :class:`ModelSpecification`. For
                example, if a translational coordinate in the specification had a ``default_value`` of ``1.0`` then
                that would be written into the :class:`ModelState` returned by this function.

                This function does not guarantee the returned :class:`ModelState`'s :class:`ModelStateStage`.
                Therefore, it's recommended that callers perform any state modifications they need followed by
                calling :meth:`realize` with a :class:`ModelStateStage` that's suitable for their desired
                needs (e.g. user interfaces may need :attr:`ModelStateStage.REPORT`).
            )"
        );
        model_class.def(
            "realize",
            &Model::realize,
            nb::arg("model_state"),
            nb::arg("model_state_stage"),
            R"(
                Realizes ``model_state`` to the desired ``model_state_stage``, which modifies
                ``model_state`` in-place.

                "Realization" of the state involves taking a new set of values from the :class:`ModelState`
                and performing computations that those new values enable. Realization is performed
                in-order one :class:`ModelStateStage` at time. For example, :attr:`ModelStateStage.POSITION`
                is realized before :attr:`ModelStateStage.VELOCITY`,then :attr:`ModelStateStage.DYNAMICS`,
                and so on.
            )"
        );
        model_class.def(
            "get_coordinate_value",
            &Model::get_coordinate_value,
            nb::arg("model_state"),
            nb::arg("coordinate"),
            R"(
                Returns the value of the corresponding state variable in ``model_state`` for the
                coordinate identified by ``coordinate``.
            )"
        );
        model_class.def(
            "set_coordinate_value",
            &Model::set_coordinate_value,
            nb::arg("model_state"),
            nb::arg("coordinate"),
            nb::arg("value"),
            R"(
                Sets corresponding state variable in ``model_state`` for the coordinate identified by
                ``coordinate`` to ``value``.

                Changing the value of a coordinate changes ``model_state``'s :class:`ModelStateStage` to
                :attr:`ModelStateStage.POSITION`. Therefore, you may need to use :meth:`realize` to
                re-realize the state to a later stage if you intend on using the state with a method that
                requires a later stage (e.g. rendering).
            )"
        );
        model_class.def_prop_ro(
            "num_outputs",
            &Model::num_outputs,
            R"(
                Returns the number of outputs the model has.
            )"
        );
        model_class.def_prop_ro(
            "outputs",
            &Model::outputs,
            R"(
                Returns a list of all outputs the model has.
            )"
        );
        model_class.def(
            "get_output_value",
            [](const Model& model, const ModelState& model_state, const Symbol& output)
            {
                return to_python_output(model.get_output_value(model_state, output));
            },
            nb::arg("model_state"),
            nb::arg("output"),
            R"(
                Returns the value of ``output`` for the given ``model_state``.
            )"
        );

        nb::class_<ModelState> model_state_class(
            _core_module,
            "ModelState",
            R"(
                Represents a single state of a :class:`Model`.

                A :class:`ModelState` bundles together the state variables, cache variables, and other information
                necessary to describe a single state of a :class:`Model`. :class:`Model`\s can read/manipulate
                :class:`ModelState`\s in order to :meth:`Model.realize` the state to a later stage
                (e.g. as part of forward integration) or read outputs values. However, :class:`ModelState`\s may
                also be created, read, and manipulated by downstream Python code and other utilities in OPynSim.
            )"
        );
        model_state_class.def_prop_ro(
            "stage",
            &ModelState::stage,
            R"(
                Returns the current :class:`ModelStateStage` of the state.

                Notes:
                    A state may be realized to a later stage with :meth:`Model.realize`.
            )"
        );

        static_assert(osc::num_options<ModelStateStage>() == 9);
        nb::enum_<ModelStateStage> model_state_stage_class(
            _core_module,
            "ModelStateStage",
            R"(
                Represents a stage of state realization (computation).

                When calling methods like :meth:`Model.realize`, a :class:`ModelState` is
                realized in-order through each :class:`ModelStateStage`, starting at the lowest
                stage and ending at the highest stage.

                Each time a :class:`ModelState` advances through a :class:`ModelStateStage`, more
                information is available in the state. For example, after a :class:`ModelState` is
                realized to :attr:`ModelStateStage.POSITION`, positional quantities such as the positions
                of bodies and offset frames are known, and any positional output on the associated
                :class:`Model` can then extract that information from the :class:`ModelState`.

                For convenience, the :mod:`opynsim` module defines aliases for each :class:`ModelStateStage`:

                - :attr:`opynsim.STAGE_TOPOLOGY` -> :attr:`opynsim.ModelStateStage.TOPOLOGY`
                - :attr:`opynsim.STAGE_MODEL` -> :attr:`opynsim.ModelStateStage.MODEL`
                - :attr:`opynsim.STAGE_INSTANCE` -> :attr:`opynsim.ModelStateStage.INSTANCE`
                - :attr:`opynsim.STAGE_TIME` -> :attr:`opynsim.ModelStateStage.TIME`
                - :attr:`opynsim.STAGE_POSITION` -> :attr:`opynsim.ModelStateStage.POSITION`
                - :attr:`opynsim.STAGE_VELOCITY` -> :attr:`opynsim.ModelStateStage.VELOCITY`
                - :attr:`opynsim.STAGE_DYNAMICS` -> :attr:`opynsim.ModelStateStage.DYNAMICS`
                - :attr:`opynsim.STAGE_ACCELERATION` -> :attr:`opynsim.ModelStateStage.ACCELERATION`
                - :attr:`opynsim.STAGE_REPORT` -> :attr:`opynsim.ModelStateStage.REPORT`
            )"
        );
        model_state_stage_class.value("TOPOLOGY",     ModelStateStage::topology,     "System topology known.");
        model_state_stage_class.value("MODEL",        ModelStateStage::model,        "Modelling choices have been made.");
        model_state_stage_class.value("INSTANCE",     ModelStateStage::instance,     "Physical parameters have been set.");
        model_state_stage_class.value("TIME",         ModelStateStage::time,         "Time has advanced and state variables have new values, but no derived information has been calculated.");
        model_state_stage_class.value("POSITION",     ModelStateStage::position,     "The spatial positions of all bodies are known.");
        model_state_stage_class.value("VELOCITY",     ModelStateStage::velocity,     "The spatial velocities of all bodies are known.");
        model_state_stage_class.value("DYNAMICS",     ModelStateStage::dynamics,     "The force acting on each body is known, along with total kinetic/potential energy.");
        model_state_stage_class.value("ACCELERATION", ModelStateStage::acceleration, "The time derivatives of all continuous state variables are known.");
        model_state_stage_class.value("REPORT",       ModelStateStage::report,       "Additional variables useful for output are known");

        // Define convenience aliases for the enum
        _core_module.attr("STAGE_TOPOLOGY")     = model_state_stage_class.attr("TOPOLOGY");
        _core_module.attr("STAGE_MODEL")        = model_state_stage_class.attr("MODEL");
        _core_module.attr("STAGE_INSTANCE")     = model_state_stage_class.attr("INSTANCE");
        _core_module.attr("STAGE_TIME")         = model_state_stage_class.attr("TIME");
        _core_module.attr("STAGE_POSITION")     = model_state_stage_class.attr("POSITION");
        _core_module.attr("STAGE_VELOCITY")     = model_state_stage_class.attr("VELOCITY");
        _core_module.attr("STAGE_DYNAMICS")     = model_state_stage_class.attr("DYNAMICS");
        _core_module.attr("STAGE_ACCELERATION") = model_state_stage_class.attr("ACCELERATION");
        _core_module.attr("STAGE_REPORT")       = model_state_stage_class.attr("REPORT");

        _core_module.def(
            "set_log_level",
            [](int python_logging_level)
            {
                // Set the C++ logging level.
                opyn::set_log_level(to_oscar_log_level(python_logging_level));

                // Additionally, set the Python logging level (the user might forget to sync them).
                nb::module_::import_("logging").attr("getLogger")("opynsim").attr("setLevel")(python_logging_level);
            },
            nb::arg("python_logging_level"),
            R"(
                Set the global logging level for OPynSim.

                By default, OPynSim minimizes log output so that downstream Python
                applications can control their own logging configuration. However,
                some modelling components emit useful diagnostic information to the log.

                This function configures OPynSim's internal logging verbosity using
                standard Python logging levels.

                Args:
                    python_logging_level (int): A logging level from the Python (:mod:`logging`) module
                        such as ``logging.DEBUG``, ``logging.INFO``, ``logging.WARNING``, ``logging.ERROR``,
                        or ``logging.CRITICAL``.

            )"
        );

        _core_module.def(
            "example_specification_pendulum",
            opyn::example_specification_pendulum,
            R"(
                Returns a :class:`ModelSpecification` of a pendulum.

                The specification is built entirely in memory with no external data files, which
                makes it useful for debugging, example Python scripts, and documentation pages. The
                returned specification should be for a one kilogram mass suspended one meter away
                from a pin joint that is one meter above the ground. For visualization, the head is
                represented as a sphere with a radius of five centimeters and the suspension rod has
                a radius of five millimeters.
            )"
        );
        _core_module.def(
            "example_specification_double_pendulum",
            opyn::example_specification_double_pendulum,
            R"(
                Returns a :class:`ModelSpecification` of a double pendulum.

                The specification is built entirely in-memory with no external data files, which makes
                it useful for debugging, example Python scripts, and documentation pages. The returned
                specification is designed to resemble the ``double_pendulum.osim``, which is available
                as an example file in `OpenSim Creator <https://www.opensimcreator.com>`_ .
            )"
        );

        _core_module.def(
            "import_osim_file",
            [](const std::filesystem::path& osim_path) { return opyn::import_osim_file(osim_path); },
            nb::arg("osim_file_path"),
            R"(
                Returns a :class:`ModelSpecification` imported from an `.osim` file on the
                caller's filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        _core_module.def(
            "add_opensim_geometry_directory",
            [](const std::filesystem::path& geometry_directory) { opyn::add_opensim_geometry_directory(geometry_directory); },
            nb::arg("geometry_directory_path"),
            R"(
                Globally adds the provided path to OpenSim's geometry search path, which
                OpenSim uses whenever it cannot find a mesh file that an OpenSim model
                file references.

                By default, relative paths in OpenSim models are resolved as ``Geometry/${mesh_path}``
                in the directory of the model file (if it's on disk). For example, if you
                have an OpenSim model file ``directory/model.osim`` that contains a ``<Mesh>``
                component with a ``mesh_file`` called ``r_pelvis.vtp``, then OpenSim will search:

                - ``directory/Geometry/r_pelvis.vtp``
                - ``${geometry_directory_path[0]}/r_pelivs.vtp``
                - ``${geometry_directory_path[1]}/r_pelivs.vtp``

                Where ``${geometry_directory_path[i]}`` is the ``i``\th ``geometry_directory_path``
                provided to this function.

                Args:
                    geometry_directory_path: An absolute path to the fallback directory.
                Raises:
                    Exception: if `geometry_directory_path` does not exist on the caller's filesystem.
            )"
        );

        _core_module.def(
            "compile_specification",
            opyn::compile_specification,
            nb::arg("model_specification"),
            R"(
                An alias for :meth:`ModelSpecification.compile`.
            )"
        );
    }
}
