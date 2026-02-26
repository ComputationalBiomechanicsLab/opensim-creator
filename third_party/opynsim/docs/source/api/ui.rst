User Interface
==============

TODO: this is roughly the direction that the API needs to go:

.. code:: python

    # Create a custom visualizer.
    #
    # `opynsim` provides Python-native bindings to a 2D UI, 3D renderer,
    # and window creation system, ported from OpenSim Creator. This enables
    # writing basic visualizers from scratch - useful for model building,
    # debugging, and presentations.
    class CustomModelVisualizer(opynsim.ui.CustomVisualizer):

        # Initializes data that's retained between frames.
        def __init__(self, model, state):
            self.model = model
            self.state = state
            self.scene_cache = opynsim.graphics.SceneCache()
            self.renderer = opynsim.graphics.SceneRenderer()
            self.renderer_parameters = opynsim.graphics.SceneRendererParameters()
            self.decorations = opynsim.graphics.SceneDecorations()

        # Called by `opynsim.App` when it wants to draw a new frame.
        def on_draw(self):

            # Clear previous frame's decorations
            self.decorations.clear()

            # Generate new decorations for the given model+state and write
            # them to `self.decorations`.
            opynsim.graphics.generate_decorations(
                self.model,
                self.state,
                self.decorations,
                scene_cache=self.scene_cache,
                decoration_options=self.decoration_options
            )

            # Add additional decorations.
            self.decorations.add_arrow(
                begin=np.array([0.0, 1.0, 0.0]),
                end=np.array([0.0, 2.0, 0.0]),
            )

            # Setup rendering parameters.
            #
            # A renderer takes a sequence of 3D decorations (`SceneDecorations`) and
            # turns them into a 2D image. `opynsim`'s rendering conventions closely
            # follow how game engines handle rendering (e.g. search/LLM for "what is a
            # view matrix in 3D rendering?").
            self.renderer_parameters.view_matrix = opynsim.maths.look_at(
                from=np.array([1.0, 0.0, 1.0]),
                to=np.array([0.0, 3.0, 0.0]),
                up=np.array([0.0, 1.0, 0.0])
            )
            self.renderer_parameters.projection_matrix = opynsim.maths.perspective(
                vertical_fov=180.0,
                aspect_ratio=self.aspect_ratio(),
                z_near=0.1,
                z_far=1.0
            )
            self.renderer_parameters.dimensions = self.dimensions()
            self.renderer_parameters.pixel_density = self.pixel_density()  # HiDPI

            # Render the scene to a 2D `RenderTexture` on the GPU.
            render_texture = self.renderer.render(decorations, self.renderer_parameters)

            # Blit (copy) the 2D `RenderTexture` to the visualizer to present it to
            # the user.
            self.blit_to_visualizer(render_texture)

    # Create a visualizer.
    visualizer = CustomModelVisualizer(model, state)

    # Continually show the visualizer until it's closed.
    app.show(visualizer)

    # OPynSim also packages a complete visualization and UI framework, ported
    # from OpenSim Creator.
    opynsim.ui.view_model_in_state(model, state)