#include "core.h"

#include <opynsim/_core/graphics.h>
#include <opynsim/_core/tps3d.h>
#include <opynsim/_core/ui.h>

#include <liboscar/platform/log_level.h>
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
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

using namespace opyn;

namespace nb = nanobind;

namespace
{
    osc::LogLevel to_oscar_log_level(int python_logging_level)
    {
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

    std::unique_ptr<OPynSimApp> g_lazy_loaded_app;
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
    // Libraries should be quiet by default - unless there's an error
    opyn::set_log_level(osc::LogLevel::err);

    // Install an exit handler that cleans up any lazy-loaded application state
    // when the Python interpreter shuts down
    nb::module_::import_("atexit").attr("register")(nb::cpp_function(&destroy_lazy_loaded_opynsim_app));

    // Globally initialize the opynsim API (Simbody, OpenSim, oscar)
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

        nb::class_<ModelSpecification> model_specification_class(
            _core_module,
            "ModelSpecification",
            R"(
                A high-level specification for an :class:`opynsim.Model`.

                A :class:`ModelSpecification` is what Python code can manipulate, scale, and customize
                before passing it to :func:`opynsim.compile_specification`, which returns a readonly
                :class:`opynsim.Model`.

                Notes:
                    OPynSim's API design separates the specification of a model (:class:`ModelSpecification`)
                    from its validated, assembled, and optimized simulation representation (:class:`Model`) to ensure
                    that the compilation process (:func:`compile_specification`) can freeze and optimize internal
                    datastructures at a single point in the process. This is in contrast to OpenSim, which handles
                    both concerns with a single ``OpenSim::Model`` class, which results in edge-cases, such as
                    incorrectly being able to edit a model after a physics system has already been assembled
                    from it.
            )"
        );
        model_specification_class.def(nb::init<>());  // Define default constructor

        nb::class_<Model> model_class(
            _core_module,
            "Model",
            R"(
                A validated, optimized, compiled, and ready-to-simulate model of a physics system.

                A :class:`Model` can only be created from a :class:`ModelSpecification` via the
                :func:`compile_specification` function. Therefore, editing a :class:`Model` requires editing its
                associated :class:`ModelSpecification` and recompiling it to create a new :class:`Model`.
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

                Notes:
                    State realization is a concept that OPynSim inherited from `Simbody <github.com/simbody/simbody>`_, which
                    has a much more comprehensive explanation of the realization process in its `Simbody Theory Manual <https://github.com/simbody/simbody/blob/master/Simbody/doc/SimbodyTheoryManual.pdf>`_. You
                    should read that manual if you want to know more.
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
                :attr:`ModelStateStage.POSITION`. Therefore, you may need to use :meth:`Model.realize` to
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
            &Model::get_output_value,
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
                necessary to describe a single state of a :class:`Model`. :class:`Model`'s can read/manipulate
                :class:`ModelState`\s in order to :meth:`opynsim.Model.realize` the state to a later stage
                (e.g. as part of forward integration) or read outputs values. However, :class:`ModelState`\s may
                also be created, read, and manipulated by downstream Python code and other utilities in OPynSim.
            )"
        );
        model_state_class.def(
            "stage",
            &ModelState::stage,
            R"(
                Returns the current :class:`ModelStateStage` of the state.

                Notes:
                    A state may be realized to a later stage with :meth:`Model.realize`.
            )"
        );

        static_assert(osc::num_options<ModelStateStage>() == 6);
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
            )"
        );
        model_state_stage_class.value("TIME",         ModelStateStage::time,         "Time has advanced and state variables have new values, but no derived information has been calculated.");
        model_state_stage_class.value("POSITION",     ModelStateStage::position,     "The spatial positions of all bodies are known.");
        model_state_stage_class.value("VELOCITY",     ModelStateStage::velocity,     "The spatial velocities of all bodies are known.");
        model_state_stage_class.value("DYNAMICS",     ModelStateStage::dynamics,     "The force acting on each body is known, along with total kinetic/potential energy.");
        model_state_stage_class.value("ACCELERATION", ModelStateStage::acceleration, "The time derivatives of all continuous state variables are known.");
        model_state_stage_class.value("REPORT",       ModelStateStage::report,       "Additional variables useful for output are known (optional: not required for integration).");

        _core_module.def(
            "set_logging_level",
            [](int python_logging_level) { opyn::set_log_level(to_oscar_log_level(python_logging_level)); },
            nb::arg("python_logging_level"),
            R"(
                Set the global logging level for OPynSim.

                By default, OPynSim minimizes log output so that downstream Python
                applications can control their own logging configuration. However,
                some modelling components (particularly those originating from OpenSim)
                emit useful diagnostic information to the log.

                This function configures OPynSim's internal logging verbosity using
                standard Python logging levels.

                Args:
                    python_logging_level (int): A logging level from the Python (:mod:`logging`) module
                        such as ``logging.DEBUG``, ``logging.INFO``, ``logging.WARNING``, ``logging.ERROR``,
                        or ``logging.CRITICAL``.

            )"
        );

        _core_module.def(
            "example_specification_double_pendulum",
            opyn::example_specification_double_pendulum,
            R"(
                Returns a :class:`ModelSpecification` of a double pendulum.

                The specification is built entirely in-memory with no external data files, which makes
                it useful for quick debugging sessions, example Python scripts, and documentation pages. The
                returned specification is designed to resemble the ``double_pendulum.osim``, which is available
                from the `OpenSim models repository <https://github.com/opensim-org/opensim-models>`_ and as
                an example file in `OpenSim Creator <https://www.opensimcreator.com>`_ .
            )"
        );

        _core_module.def(
            "import_osim_file",
            [](const std::filesystem::path& osim_path) { return opyn::import_osim_file(osim_path); },
            nb::arg("osim_file_path"),
            R"(
                Returns a :class:`ModelSpecification` imported from an OpenSim (.osim) file on the
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
            )"
        );

        _core_module.def(
            "compile_specification",
            opyn::compile_specification,
            nb::arg("model_specification"),
            R"(
                Compiles a :class:`ModelSpecification` into a :class:`Model`.

                The compilation process:

                - Validates the :class:`ModelSpecification`'s components (properties, subcomponents,
                  and sockets), throwing an exception if the specification is invalid in some way.
                - Assembles a physics system from the validated specification, throwing an exception
                  if the physics system cannot be assembled (e.g. if the contains impossible-to-satisfy
                  joints, or invalid muscle definitions).

                Raises:
                    RuntimeError: If the compilation process failed in some way. It is assumed that
                        the provided :class:`ModelSpecification` is valid.

            )"
        );
    }
}
