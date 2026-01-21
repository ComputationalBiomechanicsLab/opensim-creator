Building OPynSim As a C++ Library
=================================


.. warning::

    **The OPynSim C++ API is unstable.** This guide is aimed at C++ developers that
    accept the risks.
    
    Apart from a few patches, the OpenSim and Simbody API are pulled as-is from their
    central repositories, so those C++ APIs should be as stable as those upstreams. This
    means that, in principle, you can write new C++ codes for OpenSim/Simbody using
    OPynSim's (easier) installation and then upstream the code to OpenSim's and
    Simbody's repositories.

    The OPynSim C++ API (i.e. anything in ``libopynsim/`` or namespaced with ``opyn::``)
    and the oscar C++ API (``liboscar/``, ``osc::``) are **internal** APIs. They
    exist to service the OPynSim python API (public) and OpenSim Creator's implementation
    (internal). Therefore, if we think it makes sense to refactor/break those
    APIs in order to better-service those needs, we will.

    All of this is to say, it's possible to use these C++ APIs in your downstream
    project, but you should probably freeze the OPynSim version you use. Otherwise,
    upgrades to OPynSim might break your C++ code. Any issues/emails/requests with
    content like 'your change broke my C++ code' will be responsed with something
    along the lines of 'LOL RTFM'.


Walkthrough
-----------

OPynSim is an infrastructural project that's designed to be built from source
on Linux, Windows, and macOS. There's a cmake build for the 3rd-party dependencies
in ``third_party`` and there's a cmake build for the OPynSim project in the root
of the repository.

Here's an example bash script for building and installing everything from source:

.. code-block:: bash

    #!/usr/bin/env bash

    build_type=Release          # alternatively: RelWithDebInfo/Debug/MinSizeRel
    build_dir=${PWD}/build      # build   it in OPynSim's source directory
    install_dir=${PWD}/install  # install it in OpynSim's source directory

    # Get OPynSim's source code (incl. all third-party code)
    git clone https://github.com/opynsim/opynsim

    # Change into the OPynSim directory
    cd opynsim/

    # Build + install OPynSim's third-party dependencies
    cmake -G Ninja -S third_party/ -B ${build_dir}/third_party -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_INSTALL_PREFIX=${install_dir}
    cmake --build ${build_dir}/third_party

    # Build + install OPynSim (excl. Python bindings)
    cmake -G Ninja -S . -B ${build_dir} -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_PREFIX_PATH=${install_dir} -DCMAKE_INSTALL_PREFIX=${install_dir} -DBUILD_TESTING=OFF -DOPYN_BUILD_PYTHON_BINDINGS=OFF
    cmake --build ${build_dir} --target install

Once you've done that, ``${install_dir}`` will then contain a native install of
OPynSim and all of its dependencies (e.g. Simbody, OpenSim, Oscar). Downstream
cmake projects can use any of these dependencies, or OPynSim, with ``find_package``:

.. code-block:: cmake

    # Example CMakeLists.txt
    
    cmake_minimum_required(VERSION 3.25)
    project(your-project VERSION 0.0.1 LANGUAGES CXX)

    # `find_package` requires that the caller either installed the
    # distribution to a standard global location (e.g. to `/usr/local/`) or
    # the caller has set `CMAKE_PREFIX_PATH` to point at a custom
    # location (e.g. `-DCMAKE_PREFIX_PATH=${install_dir}`).
    find_package(opynsim REQUIRED)

    add_executable(your-executable your.cpp)
    target_link_libraries(your-executable PRIVATE opynsim)

Once your project has an appropriate ``CMakeLists.txt``, it should sucessfully
configure, provided it's called with the appropriate arguments, for example:

.. code-block:: bash

    #!/usr/bin/env bash

    # Configure your project, but ensure `find_package` can find the OPynSim installation
    cmake -S your-project -B build -DCMAKE_PREFIX_PATH=${install_dir}

    # Build your project and run it
    cmake --build build && ./build/your-executable


Installing Extra Stuff
~~~~~~~~~~~~~~~~~~~~~~

The walkthrough outlines the most straightforward way to build and install OPynSim
from source, but downstream projects tend to require additional dependencies. The
way we recommend doing it depends on what you're planning.

If you are building for a specific architecture and operating system, and you
aren't too bothered about tainting your system with additional libraries, the
easiest way is to just install stuff on your system (e.g.
``apt-get install tensorflow-dev``). Operating system maintainers such as the Debian
foundation, Canonical, and Red Hat have already built and tested these packages
for you. Once installed, it's usually just a matter of using cmake's ``find_package`` to
pull it into your project.

If you are building across multiple architectures and operating systems, then you
either have to handle system setup (installing libraries, etc.) on a system-by-system
basis, or build from source for each target. OPynSim opts for a source build. CMake
standardizes the configure-build-install pattern. E.g. if I you wanted to add glm
to my project then you'd build it and then install it into the same directory as
OPynSim was installed:

.. code-block:: bash

    #!/usr/bin/bash

    # Get the source code
    git clone https://github.com/g-truc/glm

    # Configure the build
    cmake -S glm -B glm-build -DCMAKE_INSTALL_PREFIX=${install_dir}

    # Build + install it into ${install_dir}
    cmake --build glm-build --target install


As with OPynSim, you would then provide a ``-DCMAKE_PREFIX_PATH=${install_dir}``
which would make ``find_package`` capable of finding ``glm``.


Basic Model Rendering Example
-----------------------------

.. warning::

    **Again, the OPynSim C++ API is unstable.** This guide is showing what's possible
    with the current internal API. Freeze your OPynSim version if you're building
    something that needs to survive a longer time.

Below is a code snippet for an application that can visualize an ``OpenSim::Model`` and
``SimTK::State`` pair in a window with some basic interactions:

.. code-block:: c++

    #include <liboscar/oscar.h>                                     // Include all oscar symbols (osc::)
    #include <libopynsim/graphics/open_sim_decoration_generator.h>  // osc::GenerateModelDecorations
    #include <libopynsim/init.h>                                    // opyn::init()
    #include <OpenSim/Simulation/Model/ModelVisualizer.h>           // OpenSim::ModelVisualizer
    #include <OpenSim/Simulation/Model/Model.h>                     // OpenSim::Model

    #include <filesystem>                                           // std::filesystem::path
    #include <optional>                                             // std::nullopt
    #include <vector>                                               // std::vector

    namespace
    {
        class ModelVisualizerScreen final : public osc::Widget {
        public:
            explicit ModelVisualizerScreen(const std::filesystem::path& model_path) :
                model{model_path.string()}
            {
                model.buildSystem();
                SimTK::State& state = model.initializeState();
                model.equilibrateMuscles(state);
                model.realizeReport(state);
            }
        private:
            bool impl_on_event(osc::Event& e) override
            {
                return ui_context_.on_event(e);  // Forward application events into the 2D UI
            }

            void impl_on_draw() override
            {
                // Clear the application window and tell the 2D UI context (osc::ui::) that a
                // new frame is starting.
                osc::App::upd().clear_main_window();
                ui_context_.on_start_new_frame();

                // Update the scene camera state based on the user's inputs.
                osc::ui::update_polar_camera_from_all_inputs(
                    camera,
                    osc::Rect::from_origin_and_dimensions({}, osc::App::get().main_window_pixel_dimensions()),
                    std::nullopt
                );

                // Create 3D decorations for the given model + state.
                const SimTK::State& state = model.getWorkingState();
                std::vector<osc::SceneDecoration> decorations = osc::GenerateModelDecorations(cache, model, state);

                // Add additional custom decorations to the decoration list. For example, this
                // adds a big transparent cone to the scene.
                decorations.push_back(osc::SceneDecoration{
                    .mesh = cache.cone_mesh(),
                    .shading = osc::Color::green().with_alpha(0.5f),
                });

                // Set the parameters used to render an image of the decorations:
                //
                //     .dimensions           render it as large as the application window (in device-independent pixels)
                //     .device_pixel_ratio   render it with the same density of physical pixels as the user's monitor (effectively)
                //     .anti_aliasing_level  use application-wide anti-aliasing level
                //     .view_matrix          transform decorations into view-space based on the camera
                //     .projection_matrix    project the decorations onto the screen using a perspective projection
                osc::SceneRendererParams params;
                params.dimensions = osc::App::get().main_window_pixel_dimensions();
                params.device_pixel_ratio = osc::App::get().main_window_device_pixel_ratio();
                params.anti_aliasing_level = osc::App::get().anti_aliasing_level();
                params.view_matrix = camera.view_matrix();
                params.projection_matrix = camera.projection_matrix(osc::aspect_ratio_of(params.dimensions));

                // Render the decorations to an image as a `osc::RenderTexture` (held by the renderer).
                renderer.render(decorations, params);

                // Blit the render onto the screen.
                osc::graphics::blit_to_main_window(renderer.upd_render_texture());

                // Draw a 2D UI over the render.
                {
                    // E.g. pretend that these numbers came out of a calculation or something.
                    std::vector<osc::Vector2> numbers;
                    for (int i = 0; i < 100; ++i) {
                        const float v = static_cast<float>(i) / 100.0f;
                        const float x = v*(2.0f*osc::pi_v<float>);
                        const float y = sin(x);
                        numbers.emplace_back(x, y);
                    }

                    // Create a panel - most 2D UI elements must be drawn in a panel.
                    osc::ui::begin_panel("window");
                    osc::ui::draw_float_slider("camera radius", &camera.radius, 0.0f, 5.0f);

                    // Create a plot within the panel
                    if (osc::ui::plot::begin("plot", {500.0f, 200.0f})) {
                        osc::ui::plot::plot_line("line", numbers);  // Draw a line within the plot
                        osc::ui::plot::end();
                    }
                    osc::ui::end_panel();

                    ui_context_.render();
                }
            }

            // Class members are retained between rendered frames - `App` holds one
            // instance of `osc::Widget` that's drawn over-and-over.

            osc::ui::Context ui_context_{osc::App::upd()};  // Provides the state for `osc::ui::`.
            OpenSim::Model model;                           // The model being viewed.
            osc::SceneCache cache;                          // A cache of scene objects (meshes, materials).
            osc::SceneRenderer renderer{cache};             // A renderer that can draw decorations.
            osc::PolarPerspectiveCamera camera;             // A camera that is manipulated by the user.
        };
    }

    int main(int, char**)
    {
        // Initialize the opynsim API.
        opyn::init();

        // Tell OpenSim where to find geometry files.
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths("/home/adam/Desktop/opensim-creator/resources/OpenSimCreator/geometry");

        // Create an application with one window.
        osc::App app;

        // Keep showing the visualizer widget until the window is closed.
        app.show<ModelVisualizerScreen>("/home/adam/Desktop/opensim-creator/resources/OpenSimCreator/models/Arm26/arm26.osim");

        return 0;
    }
