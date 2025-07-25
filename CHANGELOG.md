# ChangeLog

All notable changes to this project will be documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Upcoming Release]

- The search term "OpenSim Creator" should now find OpenSim Creator in the start
  menu (previously, it was 'opensimcreator'; reported by @clairehammond; #1079).
- The "load raw data file" button in the preview experimental data workflow was
  changed to "load sto/mot/trc file" to clarify its purpose (reported by
  @davidpagnon, #1057).
- A small contribution section was added to the README, which directs users to
  the contribution section in the official documentation, which is also available
  in-tree (reported by @davidpagnon, #1056).
- The "load model trajectory/states" button in the preview experimental data workflow
  was changed to "load model trajectory sto/mot file" to clarify its purpose (reported
  by @davidpagnon, #1052).
- Fixed incorrect icon DPI scaling on MacOS. The algorithm that figures out the
  correct DPI scaling for icons now also takes the main window's current scaling
  mode into account.

## [0.5.25] - 2025/07/14

- Models that contain muscles that cannot be equilibrated can now be loaded. Instead
  of halting with an error message, the system will log a warning message mentioning
  the issue and continue as normal (thanks @modenaxe, #1070).
- The 'Add' context menu for an `OpenSim::GeometryPath` now contains an "Add Path Point"
  option, which operates the same way as the same option on `OpenSim::PathActuator`s (#1061).
- Mouse wheel scrolling was improved on MacOS (again) and should now feel much closer
  to the other OSes.
- The "Preview Experimental Data" workflow now filters out data series with names that
  are incompatible with `OpenSim::Component`'s naming conventions (thanks @alexbeattie42, #1068).
- Fixed "Preview Experimental Data" crashing when it loads invalid data. Instead, it now
  emits an error message to the log (thanks @alexbeattie42,#1068).
- Fixed MacOS keybindings incorrectly using CTRL in places where COMMAND should've
  been used (#1069).
- Fixed keybindings not working in the mesh importer workflow (#1072). Unfortunately, this
  reverses the "keyboard navigation between elements in the UI is now easier" feature
  that was introduced in 0.5.24.
- Internal: `SceneDecorationFlag::NoSceneVolumeContribution` was added to help support
  rendering decorations that don't contribute to the camera's auto-focus (thanks
  @PashavanBijlert, #1071).
- Internal: `osc::Rect` was heavily refactored to make it easier to use and more clearly
  state its coordinate system assumptions (Y points up, Y points down, etc.).


## [0.5.24] - 2025/06/23

- The component context menu (right-click menu) was redesigned for consistency, and is
  now able to add any component as a child of any other component, which can be useful
  when building complex models hierarchically:
  - The menu more clearly separates common functions that are possible on any
    component (or nothing, if the background is right-clicked).
  - The `Add` menu now also takes into account which component was right-clicked,
    so that the added component ends up as a child of the right-clicked one.
  - Component-specific specialized adders (e.g. 'Wrap Object' when right-clicking
    a frame, 'Add Parent Offset Frame' when right-clicking a Joint) are now part
    of the `Add` menu, with a separator between them and the `Add` functions that
    are available to all components.
  - The "Toggle Frames" context menu action was removed. It was a legacy feature added
    in Feb 2023 (#50), but has now been superseded by options in the `Display` menu and
    the model editor's toolbar (#887).
- Mouse hittesting in the 3D viewport now uses an algorithm that prioritizes
  subcomponents over parent components in the case where the mouse ray
  intersects multiple components, which makes it easier to (e.g.) select
  muscle points that are surrounded by fibers (#592).
- The "Copy Absolute Path to Clipboard" contextual action was replaced with a "Copy"
  menu that has additional functionalities (e.g. "Copy Name", "Copy Concrete Class Name").
- The `osc.toml` configuration file now supports a `model_editor/monitor_osim_changes`
  boolean option, which can be used to explicitly tell the OpenSim model editor
  whether or not to auto-reload the file when it changes on disk (defaults to
  `true`; thanks @mrrezaie, #1000).
- The documentation link shown on the splash screen or help menu is now
  set at compile time to be equal to `docs.opensimcreator.com` (#1048).
- Attempting to import an incorrect `.osim` file into the mesh importer now
  results in a log error message rather than a crashing exception
  (thanks @davidpagnon, #1050).
- `OpenSim::PathPoint`s now have the same icon as an `OpenSim::Marker`, to
  make them easier to spot in the UI/navigator.
- The lightning button in the properties panel is now clickable even if the
  model is readonly (e.g. when simulating, #777).
- SVG icons and banners now rasterize in high DPI mode when rendering to a
  high DPI monitor.
- Keyboard navigation between elements in the UI is now easier and supports (e.g.)
  using the arrow keys to move between UI elements.
- The model warper's "Export Warped Model" button now has a submenu where the
  user can view and select which directory the warped geometry should be written
  to (#1046).
- Fixed a crashing bug in mesh warper's landmark exporter where it would infinitely
  loop and write the same landmark over and over when exporting to a CSV (#1045).
- Internal: `liboscar` now accepts its font/configuration dependencies externally,
  which helps with decoupling it from OpenSimCreator's specific font/icon/configuration
  assets.
- Internal: `liboscar` now explicitly outlines that it uses a right-handed coordinate
  system and encoding format for textures (i.e. x goes right, y goes up, origin is
  bottom-left), which matches OpenGL's conventions (#1044).
- Internal: All graphics test suites now use the same test source file convention as
  the rest of the codebase (#763).


## [0.5.23] - 2025/05/26

- The center of mass visualization for `Body` components now matches how engineering
  textbooks tend to represent CoMs (#575).
- A search bar was added to the `Add` context menu, enabling users to search through
  all available components.
- The property editors shown in the `Properties` panel were cleaned up, such that they
  align better and editor buttons now have a clear `edit` and `view` annotations.
- The alignment of property editors was adjusted such that they all align on the
  left side (previously: double editors were indented slightly).
- Fixed a crash that occurred when opening a property editor that spawns an external
  panel/dialog from within the `Add Component` dialog (#1040).
- The search bar in the "Add Component" dialog was cleaned up and now matches similar
  search bars in other dialogs.
- File dialog filters now default to filtering typically-supported file extensions for
  the given prompted filetype (e.g. opening a mesh will filter `obj`, `stl`, and `vtp`;
  previously, all dialogs defaulted to 'All Files', which can be tricky when working with
  directories containing many files).
- The mesh warping workflow now has a 'swap source <--> destination' button, to make it
  possible to see what the inverse of a TPS warp looks like.
- The mesh warping workflow now has a `source/destination landmarks prescale` option, which
  enables multiplying each landmark by a scaling factor before using them in the TPS technique.
  This matches a similar feature in the model warper and is necessary when handling data in
  different units (e.g. millimeters vs. meters).
- The mesh warping workflow now has toggles for `scale`, `rotation`, `translation`, and `warp`,
  which lets users toggle those parts of the TPS technique in-UI. This matches a similar feature
  in the model warper and is useful for understanding the underlying TPS warp.
- Fixed an edge-case where loading multiple model files simultaneously could sometimes
  cause the models not to load (#1036).
- The draft explaining `StationDefinedFrame`s has been upgraded to a full tutorial in the
  OpenSim Creator documentation.
- The test suite for `liboscar` now works in Debug mode with MSVC (OpenSimCreator doesn't
  yet, due to upstream issues in OpenSim, #982).
- The development documentation now outlines OpenSim Creator's release process and the
  exact compiler versions etc. that the project is built with (#1022 #1017).
- The original (deprecated/prototype) version of the model warper workflow was dropped.
  References to it have been replaced with references to the new model warper, which follows
  our intended long-term design goals for the feature.
- The frame definition tab button was removed from the splash screen, this is the next stage
  of deprecation after labelling it as deprecated (if you use it, write something in issue #951).
- Internal: the source code level for the project was upgraded from C++20 to C++23.
- Internal: fixed a regression introduced by an ImGui upgrade that prevents the screenshot
  taker from working if a modal dialog is shown (#1038).
- Internal: googletest was updated to v1.17.0, lunasvg was updated to v3.3.0, SDL was updated
  to v3.2.14, and stb was updated to its latest commit (802cd45).


## [0.5.22] - 2025/04/25

- The camera's auto-focus functionality now filters out any geometry that's in the
  scene, but hidden (e.g. geometry that can be clicked, rim-highlighted, etc. but
  isn't actually drawn, #1029).
- The (experimental) "Plane Contact Forces" visualization now also works when the
  `ContactHalfSpace` is not the first element of the `HuntCrossleyForce` (previously,
  it would produce incorrect visualizations because it assumed the plane was the first
  member of the contact set, #1026).
- Fixed the 'is visible' toggle on `Appearance` properties was not updating the model (#1028).
- Fixed a bug where plotting a 1D output against another 1D output (e.g. for a phase
  diagram) would plot the first against itself, which would always create a diagonal
  line (#1025).
- The `Watch Output` menu now also shows all record outputs from `OpenSim::Force`s,
  which makes it possible to (e.g.) watch `OpenSim::BushingForce`forces/torques while
  editing a model.
- Moment arms in the muscle plotter now have units of meters (previously, moment arms
  were listed as 'Unitless', #1014).
- The graphics implementation was updated to support a wider range
  of material property types (e.g. arrays of color buffers).
- The model warper (V3) now supports chaining/mixing TPS warps/substitutions of meshes
  (previously, it was one or the other).
- The model warper (V3) now supports `RecalculateWrapCylinderRadiusFromStationScalingStep`
  for rescaling a `WarpCylinder`'s radius based on stations in the model (#1013).
- The model warper (V3) now supports `RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep`
  for recalculating a `WrapCylinder`'s orientation based on stations in the model (#1016).
- Internal: the screenshot tab is now working again!


## [0.5.21] - 2025/02/28

- Modal popups (e.g. add body, add component) now immediately pop up with no
  background fade-in, because the fade-in can be uneven when the UI is running
  in an event-driven mode.
- Replaced the keybind `Ctrl+A` with `Escape` for clearing the selection in the model
  editor tab to make it consistent with other workflow keybinds.
- The main menu now contains a `Close` button, which will close the currently-opened
  tab.
- The binary packages created by the OpenSim Creator build have a new naming
  convention (#975):
  - In most cases, it's `opensimcreator-$version-$os-$arch.$ext` (e.g. `opensimcreator-0.5.21-macos-arm64.dmg`)
  - In Debian/Ubuntu's case, it's `opensimcreator_$version_$arch.deb`
  - This is to accommodate additional packages in the future (e.g. ARM64 on
    Windows, portable installers)
  - Previous releases have been retrospectively renamed to follow this convention, to
    make it easier to automatically archive/search them.
- The model editor UI now tries to revert/rollback the model when deleting components
  using the `DEL` hotkey (previously: it would crash to an error tab) #992, #991.
- The mesh importer now has a "Reload Meshes" button which, when pressed, will cause
  the implementation to immediately reload all meshes in the scene from disk (#999).
- Fixed a bug where directly opening an `.osim` from the command line/UI shell
  could sometimes result in gigantic icons (#997).
- HighDPI support has now been rolled out to 3D UI elements (previously, it was
  only rolled out to 2D UI elements - #950), #1001.
- A new configuration option, `graphics.render_scale`, can now be set in
  `osc.toml`. This configuration option can be used to upscale/downscale
  3D UI elements by a scale factor, e.g. to combine a HighDPI 2D UI with
  LowDPI 3D viewports (#1001).
- `oscar_demos` are no longer bundled with the application by default, to reduce
  the size of the installer (most users don't use the demos).
- Internal: Updated internal opensim-core to include the upstream change to
  `ExpressionBasedBushingForce`, which will hide the frames if the user sets
  the appropriate visual properties to zero (previously: would NaN them and cause
  rendering issues).
- Internal: switched from using `git submodule` to using `git subtree` to handle
  all external code, in order to make long-term archiving of OpenSim Creator's
  source code easy (everything is in-tree).
- Internal: ModelWarperV3 TPS warps now have options for configuring the input
  mesh/landmark prescale and whether or not to use the translational/rotational
  part of a TPS warp.
- Internal: ModelWarperV3 can now export the model to the model editor (hackily:
  it writes warped meshes to `${MODEL_DIR}/WarpedGeometry`).
- Internal: Almost all UI classes have been refactored over to use the `osc::Widget`
  API, so that various parts of the UI codebase can work more generically.


## [0.5.20] - 2025/01/28

- Fixed a GTK3 symbol collision between SDL3 and nativefiledialog that causes
  opening a file dialog to crash the application in Ubuntu (#993)
- Internal: the file dialog (open/save/save as, etc.) is now handled by a mixture of
  `SDL3` and `nativefiledialog` (previously, just `nativefiledialog`). The intention
  is that all usages will be moved over to `SDL3` over the next few releases.


## [0.5.19] - 2025/01/24

- The application now has a MacOS-specific logo that more closely follows Apple's icon
  guidelines.
- Negative scale factors are now supported by the 3D renderer, which can be handy for
  mirroring meshes (thanks @carlosedubarreto, #974).
- `ExpressionBasedBushingForce` in OpenSim Creator's fork of OpenSim was patched to not
  emit its own, custom, frame geometry and, instead, rely on model-level frame geoemtry
  rendering. Additionally, it was patched to handle zeroed `moment_visual_scale`,
  `force_visual_scale` and `visual_aspect_ratio` better.
- `imgui_base_config.ini` is now optional, and should now be located at
  `resources/OpenSimCreator`, rather than in the root `resources/` directory.
- Fixed a bug in upstream OpenSim Core where the `is_visible` flag of `ContactGeometry`
  was being ignored (thanks @carlosedubarreto, #980).
- Fixed a bug in upstream simbody where STL files were having the square of the number
  of normals added, causing massive memory usage (#987).
- The graphics backend will now filter out any geometry that's emitted by OpenSim that
  has a NaN anywhere in the decoration's transform (#976).
- The graphics backend will now override the color of all elements of a `SimTK::DecorativeFrame`
  when it's emitted from OpenSim/simbody with a custom, non-white, color (#985).
- Internal: fixed a bug where the screenshot tab was unable to render the screenshot
  to an image file (#981).
- Internal: the lunasvg dependency no longer installs files that include absolute paths
  on the developers/build machine's filesystem


## [0.5.18] - 2025/01/16

- The project is now buildable on Apple Silicon. Installers for both of Apple's
  supported architectures (legacy intel and apple silicon) will be supplied separately.
- Custom icons have been rolled out to various parts of the UI. The icons are currently
  work-in-progress, and this change marks a "now they're integrated into the icon
  atlas" milestone.
- Using the mouse wheel to zoom into a 3D scene now has a maximium step size, lower
  (0.1 m), and upper (1 km) limit, which should make it behave better in MacOS (#971)
- When using a 3D gizmo to move a physical offset frame that's constrained because
  it's child of a joint, a tooltip will now appear next to the user's mouse explaining
  the manipulation (i.e. it's manipulating the parent of the child, #955, #966).
- When using a 3D gizmo to move a joint center, an additional tooltip will now appear
  next to the user's mouse explaining that both joint frames are being simultaneously
  manipulated.
- The `Display` context menu now also shows model-level toggles, such as toggles
  for frame visualization (#966).
- The `Watch Output` context menu now only shows the outputs of the right-clicked
  component, rather than traversing through each parent of the component. This was
  done to simplify the menu.
- The `Sockets` context menu now lists the type of conectee that the socket supports.
- The navigator panel was tidied up and now shows a slightly different color per-row
  to make it easier to read.
- The `resources` directory was cleaned up and reorganized to try clearly separate
  the resources required by `OpenSimCreator`, `oscar`, and `oscar_demos`.
- Fixed a bug where dragging the mouse over a muscle plot panel caused the dropline
  to flicker between a "hovering" and "not hovering" state (#967).
- Fixed a bug where plotting one output against another output would cause a mutex
  recursion exception or crash the application with a deadlock (#969).
- Fixed ImGui showing debug highlights on release builds (#964).
- The documentation was updated to clarify that MacOS builds now only work against
  MacOS Sonoma or above (previously, it was incorrectly documented as Ventura or
  above).
- Internal: ImGui, ImGuizmo, and ImPlot are now entirely encapsulated in the `oscimgui`
  abstraction, which should shield OSC against external API changes.
- Internal: ImGui no longer directly talks to SDL. Instead, all of its OS communication
  goes via OSC APIs, which should help centralize OS interactions and shield the UI
  against external API changes (e.g. virtual pixels, OSC event types, etc.).
- Internal: Smaller, IO- and system-independent library code was vendored in-tree, in
  order to make building `oscar` easier and less dependent on `git submodule`s (the
  longer term intention is to start patching some libraries with `oscar`-specific
  features).
- Internal: The project's source code tree was reorganized from separate top-level
  directories (i.e. `apps/`, `src/`, `tests/`) to per-project directories (i.e. `liboscar/`,
  `liboscar_demos/`, `libOpenSimCreator/`) so that unit test code is next to the units
  it's testing and to make it easier to add new source files (cmake uses globbing now),
  along with making room for top-level integration/functional/packaging tests in `tests/`


## [0.5.17] - 2024/12/11

- Several (potentially, but unlikely, breaking) changes to the shared `geometry/`
  directory and example models were made. This is to accomodate minimizing the
  shared geometry directory. Looking forward, model designers should prefer a
  model-local `Geometry/` directory:
  - The following ellipsoid mesh files were deleted. None are used by any example
    model. Prefer an `<Ellipsoid>` geometry component in your model to these:
    - `ellipsoid.vtp`
    - `ellipsoid_center.vtp`
    - `ellipsoid.stl`
    - `ellipsoid_center.stl`
  - Meshes related to the `SoccerKickingModel.osim` are now model-local (i.e. were
    copied to a `Geometry/` directory next to the model). It is unlikely that models
    in the wild used (e.g.) the football mesh. If yours did, it can be fixed by adding
    the relevant meshes to a model-local `Geometry` directory.
  - The `SoccerKickingModel.osim` meshes were decimated and converted to OBJs to minimize
    their disk usage (previously: the football was 1.5 MB).
  - The `Fly` model (+meshes) was dropped from the examples. This model was only present
    for internal OpenSim Creator testing (e.g. #116, #616) and contained nothing of
    research interest (muscles, correct joints, etc.).
- The 2D backend was updated to support HighDPI scaling (3D is WIP). This means that
  the `[experimental_feature] high_dpi_mode` in the configuration is now always `true`.
- The Coordinates panel now contains a `Pose` dropdown that exposes the ability to zero
  all joint coordinates in the model (thanks @tgeijten, #959).
- The socket reassignment popup was given a makeover, and tries to draw more attention
  to the `Re-Express $COMPONENT in new frame` option (thanks @tgeijten, #957).
- The search bar in the `Navigator` panel was cleaned up slightly.
- Workflow links on the splash screen were reordered slightly.
- "Fullscreen" was dropped from the about tab. The "Fullscreen" button now always follows
  "windowed fullscreen" behavior (oldskool exclusive fullscreen usage isn't very useful
  for a tooling application).
- The "Frame Definition" workflow is now labelled as deprecated, because we anticipate
  that it isn't used very much. If you think otherwise, then post a comment on issue
  #951.
- The documentation now contains an explanation for how to install OSC on MacOSes
  that have a newer, more draconian, gatekeeper (#942).
- The documentation banner now includes the text "OpenSim Creator", rather than
  just showing the OpenSim Creator logo.
- A CC-BY license was added to `resources/icons` (#944).
- Fixed a regression where the `Display` context menu would always be greyed out in
  the model editor (thanks @tgeijten, #958).
- Fixed 'Wireframe' appearance causing the component's decoration to disappear, rather
  than showing as a wireframe (#952).
- Fixed a typo where tutorial 5 referred to a "femur CT scan" when the content was
  about a "pelvis MRI scan" (#949).
- Fixed the `deb` package not listing all of its runtime dependencies (e.g. `libopengl0`),
  which made installing OpenSim Creator on fresh debian VMs more difficult.
- Internal: `imgui`, `implot`, `lunasvg`, and `stb` were updated (#948)
- Internal: UI panels now uniformly use the `osc::Panel` and `osc::Widget` APIs.
- Internal: the platform backend was changed from SDL2 to SDL3, which has better
  HighDPI and file dialog support (#950).
- Internal: a prototype of a new model warping UI was added in this release. It isn't
  ready for production yet, but should make supporting features like only warping the
  `translation` part of an offset frame (#894) or explicltly assigning specific TPS
  warps to specific stations in the model (#891) possible - once it works ;)
- Internal: `oscar_demo` resources were JPEG compressed at a lower quality (the assets
  are really only for development and don't need to look perfect).
- Internal: The OpenGL loader was switched from `glew` to `glad` to make the
  build simpler and remove a dependency on GLX.


## [0.5.16] - 2024/11/04

- The `.vtp` files that are shipped with the demo models are now 30 % smaller,
  because they no longer contain unused normals (opensim-models/#181, #941)
- The asset files for `oscar_demos`, which are shipped with the installer, are
  now compressed with jpg, which makes the installer much smaller.
- There is now a `Color Scaling` option in the muscle coloring/styling panel, which
  enables auto-scaling the min/max (e.g. blue-to-red) color range based on the model-wide
  min/max of the chosen `Color Source`. This is useful for visualizing differences between
  muscles that are all within a tight numeric range (#933).
- Renamed the `OpenSim` options in the muscle styling panel to what they actually are
  (e.g. `OpenSim` coloring is now `Activation`, `OpenSim` rendering is now `Lines of Action`,
  #933).
- The OpenSim Creator executable is now built with static linking which, with
  a little more work, will enable portable executables (#881).
- The installation, building, development environment setup, and contribution guides
  have been moved into the documentation pages (https://docs.opensimcreator.com),
  so that there's a centralized location for all documentation.
- Finished off tutorials 5 and 7 in the documentation.
- After dragging a file into OSC, the next file dialog that opens should now default
  to the same directory that the files were dragged from (thanks @mrrezaie, #918).
- Fixed a bug where the model manipulation gizmo would show on readonly models (e.g.
  in the simulator tab, #936)
- Fixed a bug where editing a model that has an associated trajectory loaded in the
  preview experimental data workflow would revert the model to its `t=0` state (#932)
- Internal: OpenSimCreator now uses builds the OpenSim API via `osim` (https://github.com/ComputationalBiomechanicsLab/osim),
  which makes building + packaging easier for the project
- Internal: `oscar_simbody` was re-integrated into `OpenSimCreator` to reduce the amount of
  cmake faffing around going on in the project
- Internal: `lua` and `ImGuiColorTextEdit` were dropped as dependencies because they were
  only used by one demo. They might be re-added later, if we get enough time to integrate
  in-engine scripting ;)


## [0.5.15] - 2024/10/07

- A `Preview Experimental Data` workflow has been added. This is **work in progress**, but
  ultimately has a similar intent to the official OpenSim GUI's `Preview Experimental Data...`
  workflow, with a few twists:
  - Similarly to OpenSim GUI, you can load an `.osim` model, associated `.osim` motion
    (`.sto`/`.mot`) and raw data into the scene. However, by default, all data will be
    overlaid with no spatial offset, and time scrubber will scrub all motion tracks, rather
    than one of them. This helps with validating several datastreams (e.g. an IK result
    against input marker motions).
  - Additionally, it can preview the effect of adding `ExternalLoads` and `PrescribedForce`s
    XML files to the model. You'll need to enable the `Point Forces` or `BodyForces` visual
    aid in the 3D visualizer panel to visualize those forces.
  - The above two features can be used at the same time, which helps with (e.g.) validating
    that `ExternalLoads` are being applied to the correct frame etc.
- The simulator tab now has similar behavior to the model editor tab. For example, it is now
  possible to view the coordintes panel in the simulator tab - albeit, with editing
  disabled. This is part of an ongoing effort to merge the model editor, simulator, and preview
  experimental data workflows into a single UI.
- The function curve viewer now spawns a separate, movable, panel that can be docked in the
  editor UI. Previously, it would spawn a popup dialog, which would prevent the ability to edit
  the model while viewing a function curve. With this change, it is now also possible to view
  multiple function curves at the same time.
- The 3D manipulation gizmo that pops up when selecting (e.g.) markers and frames now defaults
  to editing in their `Local` space, rather than editing in `Global` space. This is to reduce
  confusion between "this is a UI manipulation gizmo" and "this is the frame I'm editing" in
  the case where frame visualization is disabled. Global-space editing is still possible via
  a dropdown in each 3D visualizer (or double-click `G`, `R`, etc. #928).
- It is now possible to watch/plot parts of a `SpatialVec` component output. For example, it's
  now possible to watch/plot the `reaction_on_child` and `reaction_on_parent` outputs of an
  `OpenSim::Joint` (thanks Jeremy Genter at ZHAW, #929).
- There is now a slight color difference between selected and hovered items in the UI (e.g.
  the color highlighting used in the navigator panel; thanks @mrrezaie, #916).
- The expression frame selector that appears in the property editor when editing spatial
  quantities that are expressed with respect to a parent frame now shows the parent frame's
  name by default (previously: it would show `(parent frame)`; thanks @mrrezaie, #916).
- Mousing over the barrel (i.e. non-points) of an `OpenSim::Ligament` component now coerces
  the hit to the `OpenSim::Ligament` component (previously, would select the associated
  `GeometryPath`; thanks @mrrezaie, #916).
- The `Experimental Tools` section of the `Tools` menu now contains a `Export Multibody System as Dotviz`
  option, which is handy for dumping the body/joint topology of a model to an external graph
  visualizer (e.g. https://dreampuf.github.io/GraphvizOnline; thanks @mjhmilla, #920).
- Force arrows are now half as long, to make them more manageable when forces become large.
- "Point Torques" was removed as a visualization option (it did nothing).
- Fixed a bug where "Show Forces on Bodies" was erroneously handling the body's force vector
  as-if it were in the parent frame (OpenSim's `ForceConsumer` API emits them in ground, #931).
- Fixed a bug where the navigator panel would erroneously autoscroll when a user selects a
  component in it (it should only autoscroll when the selection happens elsewhere, #908).
- Internal: OpenSim-independent simbody code was refactored into a separate `oscar_simbody`
  library, so that we can port it independently to other platforms (e.g. wasm).
- Internal: the UI datastructures offered by `oscar` are now better-equipped for handling
  custom events, which makes features like (e.g.) spawning function curve panels easier to
  implement in `OpenSimCreator`.


## [0.5.14] - 2024/09/04

- The `Navigator` panel in the model editor now automatically scrolls to the selected
  component when selecting the component via some other panel (e.g. a 3D viewer, reported
  by @mrrezaie, #908)
- Added `Point Forces` and `Point Torques` visualization options, which enables drawing force
  vectors in their point-force form (as opposed to their reduced body-force form). This feature
  currently only works for `ExternalForce`s in the model (#904) and `GeometryPath`s (#907).
- The `Show Forces' Linear/Rotation Component` option was reworded to `Forces on Bodies` and
  `Torques on Bodies` to reflect what's actually being shown
- Fixed force vector arrows not casting shadows
- The rim highlights now more-clearly distinguish between hovered, selected, hovered+selected
  elements in the scene when they are overlapping (#24)
- The (experimental) model warping UI is now available from the splash screen. Note: it's still
  in development, and things may break!
- Internal: `RenderTextureDescriptor` was refactored into a POD struct called `RenderTextureParams`
  for ease-of-use
- Internal: oscar's `Graphics/Scene` classes were refactored for ease-of-use and grouping of rim
  highlights (#24)

## [0.5.13] - 2024/07/30

0.5.13 adds support for visualizing force vectors in the model editor, the
ability to visualize `OpenSim::Function` properties (e.g. the `ActiveForceLengthCurve` of
a `MillardEquilibriumMuscle2018`), and easier visual joint parent/child/center placement
in the model editor - along with a bunch of other smaller changes/fixes!

- The model editor's/simulator's 3D visualizers now have an experimental
  `Show > Forces' Linear/Rotational Component` visualization option, which adds arrows indicating
  how each `OpenSim::Force` in the model applies its linear/angular force component to each
  body in the model. This can be useful for debugging model creation or `ExternalForce`s
- The model editor UI now has experimental support for viewing `OpenSim::Function` curves. This
  is currently exposed as an eye icon in the property editor panel (#695)
- Selecting an `OpenSim::Joint` that has `OpenSim::PhysicalOffsetFrame`s for both its parent
  and child frames now shows a 3D manipulation gizmo that lets you move the joint center
  without moving anything else in the model (#159)
- Manipulating a child offset frame of a `OpenSim::Joint` should now work correctly for both
  translation and rotation, allowing you visually place the parent of the child offset frame
  (e.g. a body) that's moved by the joint
- The UI now remembers which panels (e.g. Log, Properties) you had open between boots (#17)
- When changing selection in the model editor, the 3D gizmo manipulator will now check
  whether the new selection supports the same transformation type (rotate/translate) and
  will automatically coerce the transformation type to a supported one if the user's
  currently selected mode isn't supported (#705)
- Overlay geometry (e.g. the XY grid overlay, the axis lines overlay) are now correctly
  scaled by the `scene scale factor` (#700)
- The mesh warper UI now recalculates the result mesh's normals after applying the warp
  to the source mesh. This behavior can be disabled using a checkbox that was added to
  the UI (#743)
- The socket reassignment popup now shows the name of the component that the socket is
  currently assigned to (#770)
- The model editor toolbar now contains a toggle for toggling a model's `show_forces`
  display property (#887). Note: OpenSim only really uses this for `SmoothSphereHalfSpaceForce`
- Fixed non-default scene scale factors not being retained whenever a model file is
  hot-reloaded from disk in the model editor (#890)
- The mesh warping tab now has a `Visualization Options` menu, which lets users toggle a few
  basic visualization options (e.g. grid lines, #892)
- The model warping workflow now writes the warped meshes to disk at `$MODEL_DIR/WarpedGeometry`
  (previously: it kept them in-memory, which was only really useful for development, #889)
- The mesh input (source/destination) panels in the mesh importer now have the option to import
  generated meshes, which is handy for debugging/demos
- The [opensim-jam-org/jam-plugin](https://github.com/opensim-jam-org/jam-plugin) is
  now built alongside OpenSimCreator, which enables loading models containing
  `Smith2018ArticularContactForce` and `Smith2018ContactMesh` (#884)
- Fixed a bug where the DAE 3D scene exporter wasn't writing color information (#880)
- The `Open Model` file dialog now allows for multi-select, which opens each selected
  `.osim` file in a seperate tab
- Pressing `Ctrl+W` or `Command+W` (Mac) now closes the currently-active tab, excluding
  the splash tab
- When a model editor tab is active, it now shows the filename of the osim file--if there is
  one--in the application window's title
- The "Axis Lines" overlay option now extends the lines out to 5 meters (previously: 1 m)
- Internal: Added `src/OpenSimThirdPartyPlugins`, for housing copy+pasted OpenSim
  plugin code sourced from the internet (e.g. SimTK.org)
- Internal: Third-party dependencies were updated to their latest versions, where applicable
- Internal: `AppConfig` was dropped and all uses of it were converted to `AppSettings` (#893)
- Internal: `AppSettingValue` was refactored into a `Variant` (#789)
- Internal: Added runtime-checked pointer classes (`LifetimedPtr`, `LifetimeWatcher`,
            `ScopedLifetime`, `SharedLifetimeBlock`, `WatchableLifetime`)
- Internal: Added barebones `TemporaryFile` class (handy for testing IO-related code)
- Internal: `ImGuizmo` was mostly refactored into an internal concern, with OSC using an
  easier-to-integrate wrapper class
- Internal: `benchmarks/` were dropped (unused: microbenchmarks are better-suited to
  `opensim-core`)
- Internal: Added `SharedPreHashedString` class, as a generalization of the `StringName` class
  without the global lookup requirement
- Internal: Integrated `lua` and `ImGuiColorTextEdit`, which will currently only be used to
  experiment with scripting in a developer-facing fashion (no user-facing LUA support... yet)
- Internal: a wrapper layer was added around `ImPlot`, so that the UI code can use oscar-like
  bindings, rather than translating everything to ImPlot
- Internal: the `OpenSimCreator` test suite now also ensures that all user-facing models in the
  documentation can be loaded + rendered by `OpenSimCreator` (#844)
- Internal: Added `osc::Flags` class as a templated helper class for handling for flag-based enum
  types, inspired by `QFlags` from Qt


## [0.5.12] - 2024/04/29

0.5.12 adds support for adding a templated `CustomJoint` (i.e. a `PinJoint`-like joint), cleans up some
of the UI buttons/menus, and fixes various little issues, such as not being able to delete outputs from
the simulation screen.

- MacOS is now built using the `macos-13` (Ventura) GitHub Action runner, which means that OSC
  will only work on Ventura or newer
- Right-clicking a 2D output plot in the simulator tab now shows a context menu with the option to
  export the plot to a CSV or stop watching the output (#841)
- Fixed the `Save All` button only showing in the output plots tab if any of the plots happens to be
  1D floating-point data (it should now show when 2D data is being plotted, too, #840)
- Fixed using `Save All > as CSV` in the simulation tab when plotting 2D outputs (e.g. phase diagrams)
  wasn't exporting the 2nd dimension as a column in the CSV (#840)
- `CustomJoint`s can now be added via the UI. They default to having a single rotational degree of
  freedom along the Z axis (i.e. a `PinJoint`-like `CustomJoint`). Editing a `CustomJoint` still
  requires manually editing the `.osim` file, though
- The simulation tab now has an `Actions` menu, which includes the ability to extend the end time of
  a simulation by some multiplication factor (2x, 4x, etc. #839)
- The chequered floor texture is now rendered with `Nearest` sampling, which makes it appear sharper
  in the foreground
- Memory usage was reduced by optimizing font loading and log message storage
- The loading bar on the loading tab is now more centered and scales with the overall window size
- Fixed "Plot Against other Output" feature not working with non-model-sourced outputs, such as
  the integrator's "Step Wall Time"
- The "empty" state of some panels (e.g. "nothing selected", "nothing watched") was tidied up and
  now shows a centered message with, where applicable, a tip for showing information in the panel
- Each plot in the `Output Plots` panel of the simulator tab now shows a little trash icon, to
  match the deletion button that's shown in the model editor tab
- The right-click menu of an output plot in the simulator tab will now show a little trash icon
  and the button text `Stop Watching` (previously, `Watch Output` with a tick box) to match the
  deletion button that's shown in the model editor tab
- The `Selection Details` panel that's shown in the simulator tab was cleaned up to prioritize
  showing property values (similar to the editor tab), followed by a toggleable outputs section (#838)
- Fixed a bug in the `Selection Details` panel where drawing too many output plots would cause
  drawing to fail
- `README.md` now explicitly mentions that a C++20 compiler is required to build the project
- Internal: The project now sucessfully builds with Visual Studio 2022, XCode-15, `clang++-11`,
  `gcc-12`, and emsdk (3.1.58 - `oscar` only)
- Internal: `std::ranges` were rolled out code-base wide. Now-unused OSC shims were droppped.

## [0.5.11] - 2024/04/04

0.5.11 adds support for adding wrap objects to a model in the model editor, along with associating the
wrap object with a `GeometryPath`. It also adds support for creating 2D plots, which is handy for creating
phase diagrams during simulation.

- Right-clicking a 1D plot in the simulator tab now shows a `Plot Against Other Output` option, which
  lets you create a 2D output by combining two existing model outputs (handy for phase diagrams, etc.)
- Right-clicking on a `PhysicalFrame`/`Body` in the model editor now shows an `Add` menu that includes
  the ability to add geometry, offset frames, and wrap objects. Previously: was `Add Geometry` and
  `Add Offset Frame` were shown, but now there's also the ability to add wrap objects. (#7)
- Right-clicking a `GeometryPath` in the model now shows an `Add` menu that includes the ability to add
  a `PathWrap` to the `GeometryPath`, which is handy in conjunction with the above.
- Added `Export > Non-Participating Landmarks to CSV` as an export option to the mesh warper
- The mesh warper can now has the option to export source/destination/result meshes in OBJ format
  with/without surface normals (note: Simbody/OpenSim ignore this information, but it's useful if you
  plan on using the OBJ file in other software, e.g. Blender)
- Pressing `Ctrl/Super` and `PageUp`/`PageDown` now toggles between each tab. Alternatively, you can
  press (WindowsKey or Command) + (Alt or Option) + (Left or Right) for the same behavior (matches how
  MacOS apps tend to handle tab navigation)
- Pressing `Space` while in the simulator tab now (un)pauses the simulation
- There is now a `loop` checkbox in the simulator tab, which will cause playback to loop back from
  the start when playback hits the end of a simulation
- Internal: more of the UI code was ported from ImGui:: to ui:: to hide an implementation detail
- Internal: Added `MeshPhongMaterial`, which is handy for graphics development
- Internal: The `OutputExtractor` APIs were redesigned to support use-cases such as synthesizing
  outputs at runtime by combining other outputs (`ConcatenatingOutputExtractor`)
- Internal: The `OutputExtractor` APIs were redesigned to support emitting `string`, `float`, and
  `Vec2` outputs (previously: only string/float) to facilitate 2D plotting

## [0.5.10] - 2024/03/05

0.5.10 adds provisional support for OpenSim::StationDefinedFrame, which has now been added to OpenSim and should
become widely available with the release of OpenSim 4.6. It also adds more options to the Calculate - primarily so
that the Fit Analytic Geometry to This can be used to extract similar values to external scripts (e.g. the shape
fitters from 10.1017/pab.2020.46).

(It also fixes a scrolling bug in the mesh importer that slipped into 0.5.9 - sorry about that)

- Updated `opensim-core` to a version which includes `StationDefinedFrame` support:
  - Upstream feature: https://github.com/opensim-org/opensim-core/pull/3694
  - They let you define an `OpenSim::PhysicalFrame` from stations/landmarks/markers. This more closely
    matches ISB standards and the way in which people tend to build anatomical models.
  - Because they are an `OpenSim::PhysicalFrame`, they can be used a joint frames, offset frames, etc.
  - **BEWARE: EXPERIMENTAL FEATURE**:
    - Upstream OpenSim (+GUI) have not yet released this feature, so models that use it cannot be opened
      outside of OpenSim Creator (yet).
    - OpenSim Creator's tooling support for the feature (visualization, documentation, etc.) is currently
      limited, but we plan to improve it over time
- The `Calculate` menu for `Ellipsoid`s and `Frame`s now contain additional options, such as being able
  to ask for the axis directions, so that users can get similar information out of shape-fitted geometry
  as they would from external scripts
- The warping algorithm used by the mesh warper and model warper (TPS) now `lerp`s between "not warped"
  and "fully warped", rather than recomputing the warp kernel based on blended input data:

  - Practically, this means that setting the blending factor to zero guarantees that the warped mesh is
    exactly the same as the input mesh (previously, it would try to solve for the input positions and
    sometimes produce a non-identity solution when given a small number of input landmarks)
- Fixed mesh importer scrolling calculation for the new camera manipulator causing unnecessary scrolling
- Fixed muscles not casting shadows after the surface rendering change
- There is now a minor delay when mousing over entries in the `Add` menus in the model editor
- Internal: google/benchmark, ocornut/imgui, sammycage/lunasvg, nothings/stb, marzer/tomlplusplus, and
  martinus/unordered_dense were all updated to their latest releases


## [0.5.9] - 2024/02/27

- The mesh loader now obeys the normals of shared vertices in provided mesh files:
  - Practically speaking, it means that the surface shading of meshes in OSC will more closely match what
    you see in OpenSim GUI
  - It also means that if you smooth-shade a mesh in external tooling, then it will also be smooth-shaded
    in OSC (previously: OSC would throw away vertex duplication information and not smooth-shade meshes)
  - If you want flat-shaded meshes, you must now ensure that your meshes are flat-shaded when exported
    from other 3D tooling (e.g. in Blender, load the mesh, right-click, Shade Flat, reexport)
- When adding a new component, the socket selector now contains help markers and subtitles that document
  each socket that needs to be assigned (#822)
- When adding a new component, the socket selector now has a search input, which lets you filter through
  the available options
- The `Add Parent Offset Frame` and `Add Child Offset Frame` actions when right-clicking a joint now
  correctly attach the added offset frame to the joint and select it (#824), but you can't create chains of
  offset frames this way yet, because of a known bug in opensim-core (opensim-core/#3711)
- The camera axes widget now ensures that each axis is rendered in the correct order when two axes overlap
- The camera axes widget circles are now clickable, and will align the camera along the clicked axis
- The camera axes widget now appears in the top-right of each model editor visualizer window, which is
  consistent with (e.g.) Blender and Godot
- The visualizer now obeys the "Display Properties" of an OpenSim Component when it's set to `Wire`, which
  is more consistent with OpenSim GUI
- The `Appearance` editor in the property editor now shows a dropdown that lets you select the component's
  representation (e.g. `Wire`). Note: `Points` does not work, which is consistent with OpenSim GUI
- There is now an `Export Model Graph as Dotviz` utility under `Tools -> Experimental Tools` in the model
  editor, which produces a `.dot` file that can be visualized with external graph visualizers, such as
  Graphviz Online
- OpenSim was updated to the latest upstream `main` branch, which includes better socket connection support
  and other performance improvements and bugfixes
- The coordinate editor's sliders now enable text-input mode when double-clicked (previously: only Ctrl+Click)
- Fixed a crash when visualizing contact forces in the model editor is enabled (sorry about that :-))
- Internal: ImGui, ImPlot, and ImGuizmo were all upgraded to their latest version
- Internal: glm was dropped as an external dependency by copying the bare-minimum parts used by `osc` in-tree


## [0.5.8] - 2024/02/14

0.5.8 makes moving `PhysicalOffsetFrame`s a little bit easier when they're the child of a `Joint`, and adds
user-facing support for OSC-specific `OpenSim::Component`s (explained below). It also contains a few minor
bugfixes, plus a variety of internal engine changes.

- Editing the position of a `PhysicalOffsetFrame` that's a child of a Joint now behaves more intuitively
  (rotation still sucks, though)
- OSC-specific experimental OpenSim components are now exposed to the user via the model editor. So, as
  a user, you can now (e.g.) manually add a `PointToPointEdge` to your model in order to measure the
  distance between two points (previously: `PointToPointEdge` was only available through the
  `Frame Definition` UI)

> **Beware**: OSC's custom/experimental components are incompatible with OpenSim. The intention
>             of this feature is to provide users with a way to test rapidly-developed OSC components
>             that may, with enough interest, be upstreamed to `opensim-core`.

- The available socket options when adding a new component to the model should now show all valid
  options, rather than just showing frames in the model (#820)
- The camera's location/fov is now editable in degrees
- The camera's panning behavior is better when viewing a 3D scene via a viewport with a very skewed
  aspect ratio (e.g. when the model visualizer is tall and narrow, or short and wide)
- Updated Sphinx to version 7.2.6, and the associated sphinx-book-theme to 1.1.0, which makes the
  documentation look a little nicer
- Internal: the engine now uses strongly-typed angles (`osc::Radians` and `osc::Degrees`) to reduce
  unit mismatches in the source code
- Internal: the test suite now automatically exercises all available UI tabs (mesh warper, model
  editor, etc.) tabs to ensure they are always working
- Internal: all header files are now suffixed with `.h` to match most libraries etc. (previously: `.hpp`)


## [0.5.7] - 2024/01/11

0.5.7 is purely a bugfix release. The main fix is that the GeometryPath editor popup has
now returned. It dissapeared because `opensim-core` now supports function-based muscle
paths.

- Fixed `PathSpring` components ignoring the scene scale factor (#816)
- Fixed a bug where the property editor for a muscle's geometry path was not rendering
  the customized muscle path point editor popup (#815)
- Fixed `PathSpring` not using it's `GeometryPath`'s color correctly in OpenSim
  (#818, upstream: opensim-core/#3668)
- Internal: ImGui now renders its content via the oscar graphics API, rather
  than via its own internal OpenGL backend
- Internal: oscar pure-virtual interface classes are now prefixed with `I` (e.g.
  `ITab`, `IPanel`, etc.)
- Internal: OpenSimCreator pure-virtual interface classes are now prefixed with `I`
  (e.g. `IOutputExtractor`, `ISimulation`, etc.)


## [0.5.6] - 2023/12/22

Happy holidays!

0.5.6 tries to make it easier to get 3D datapoints in/out of various workflows. This improves interop
between mesh-based workflows (e.g. the mesh warper) and model-/scene-based workflows (osim editor, mesh
importer). Alongside those changes, the mesh warping workflow was improved with better UX and more data
import/export options.

0.5.6 also includes QoL changes that (e.g.) make `.osim`s load faster, tooltips less annoying, and filepaths
easier to copy+paste.

### General Changes

- Added `Tools -> Import Points` to the `osim` editor workflow:

  - Imports 3D datapoints defined in ground from a CSV file
  - The datapoints will be imported into the `osim` model as OpenSim markers
  - This feature is handy in conjunction with OSC's ability to re-express datapoints in frames (Right-click marker ->
    `Sockets` -> `Connectee` -> `change` -> `Re-Express in chosen frame`)
  - It's also handy in conjunction with calculating quantities (Right-click marker -> `Calculate` -> `Position`)

- The `Import Stations from CSV` popup in the mesh importer is now more permissive in what it accepts:

  - It is now able to import CSVs without a name column, or a header row.
  - Instead of refusing to import all of the data when there's a validation issue, it will instead present
    a user-facing that can be ignored (i.e. to import all rows that validated fine).

- Fixed a bug where muscle points could not be selected via a 3D viewport if the user is rendering muscles
  with tendons+fibers (#706)
- Fixed mesh files being double-loaded by the rendering backend. This should generally improve osim loading times
- Fixed an upstream bug in simbody's `vtp` parser (see simbody/simbody/#773). This should generally improve
  osim loading times - if the osim contains `vtp` meshes.
- In general, tooltips should now appear after a short delay, rather than immediately. This makes them
  less annoying when rapidly hovering over right-click menus.
- A couple of tooltips were removed from the mesh importer in cases where the tooltip was overlapping the
  submenu that the user's mouse hover opened. This was because it was annoying to have a tooltip cover the submenu.
- OSC now internally tries to only show users canonical-form filepaths. This means that more filepaths
  displayed to user should match the format used by their OS (e.g. `C:\somedir\to\model.osim` on Windows,
  `/home/user/dir/model.osim` on Linux), which makes those paths easier to copy+paste into other software
  (e.g. Windows explorer).
- Mousing over the connectee in the reassign sockets context menu now shows a tooltip that also lists the
  component type.

### Mesh Warper Changes

**tl;dr**, the mesh warping workflow was improved such that it has better data interop with the mesh
importer and model editor workflows. The UX was improved by adding more keybinds, visual feedback, and options.

- The mesh warper UI now correctly recycles landmark names - even if they were previously generated
  and deleted
- The mesh warper UI now has a toggle-able (default: off, initially) `Landmark Navigator` panel that shows
  the names and statuses of all landmarks in the scene (#807)
- Hovering over a landmark in the mesh warper UI now brightens the landmark slightly
- Landmarks in the mesh warper UI can now be renamed and repositioned via a right-click context menu (#807)
- The mesh warper's CSV exporter for landmarks now includes a header row and name column (#807)
  - The previous functionality (just positions) is exposed as "Landmark Positions to CSV"
- The mesh warper's CSV exporter for fully-paired landmark pairs now includes a name column (#807)
  - The previous functionality (no names) is exposed as "Landmark Pairs to CSV (no names)"
- The mesh warper's CSV exporter for warped non-participating landmarks now includes a name column (#807)
  - The previous functionality (of exporting no names) is exposed as "Warped Non-Participating Landmark Positions to CSV"
- The mesh warper's CSV importer can now import named landmarks (#807):
  - It's assumed that CSV files with >= 4 columns have the name in the first column
  - If a landmark with the same name is already available for mesh mesh you're importing for, it overwrites the location
  - Otherwise, it either pairs the landmark (if a landmark with same name is found for the other mesh), or creates a new
    landmark with the given name
  - The previous functionality (of handling CSV files with 3 columns) has not changed
- The mesh warper's CSV importer can now import named non-participating landmarks (#807):
  - It's assumed that CSV files with >= 4 columns have the name in the first column
  - If a non-participating landmark with the same name is already in the scene then it will overwrite it's location
  - The previous functionality (of handling CSV files with 3 columns) has not changed
- The mesh warper UI now supports more keybindings, which are now also listed next to the menu items contained within
  the the top menu bar
- Non-participating landmarks can now be added to the source mesh in the mesh warper by Ctrl+Clicking on the surface
  of the mesh
- The `clear landmarks`/`clear non-participating landmarks` buttons were moved from the toolbar to the `Actions` menu
- The mesh warper landmark navigator panel now highlights which landmarks are hovered/selected


## [0.5.5] - 2023/11/21

0.5.5 is purely a bugfix release that fixes a crash that can happen when booting OSC on MacOS (see #811, thanks
@SAI-sentinal-ai for reporting it):

- Fixed a crash on MacOS where a `std::sort` algorithm usage did not strictly comply with `Compare` (thanks @SAI-sentinal-ai, #811)
- `osc::App` now writes the `osc` executable's directory and user directory to the log (handy for debugging, #811)
- Internal: nonstd::span was droppped in favor of C++20's std::span
- Internal: C++17 was upgraded to C++20, which required some dependency patching
- Internal: all vector/matrix math was ported from using `glm` directly to using it via an `osc` alias
- Internal: opensim-core was upgraded to enable C++20 support and remove all currently-known memory leaks
- Internal: opensim-core was updated to use the commit with C++20 fixes (b4d78a2, see #3620 in opensim-org/opensim-core)
- Internal: the HACK that was used internally by OSC to handle the OpenSim bug reported in #654 was removed, because
            the opensim-core that OSC now uses contains opensim-org/opensim-core/#3546 (upstream patch to #654)


## [0.5.4] - 2023/11/03

0.5.4 adds support for basic shape fitting into the model editor, some nice-to-have orientation/translation actions in the
mesh importer, and some bugfixes.

- Stations are now selectable when using the `Translate > To (select something)` and `Reorient > $axis > To (select something)`
  actions in the mesh importer (thanks @emmccain-uva, #796)
- The mesh importer now has an option to `Reorient > $axis > Along line between (select two elements)`, which is sometimes
  useful (esp. in conjunction with importing stations, #149) for defining joint/body frames (thanks @emmccain-uva, #797)
- Right-clicking a mesh in the model editor now shows an `Export` menu, enabling re-exporting of the mesh w.r.t. a different
  coordinate system (#799)
- Right-clicking a mesh in the model editor now shows a `Fit Analytic Geometry to This` option, which will fit the chosen
  analytic geometry to the mesh's vertices (#798).
- If a popup in the editor window throws an exception then the editor window will now try to close the popup, rather than
  fully erroring out to an error tab (thanks @AdrianHendrik, #800)
- Fixed a bug where trying to add a `ConditionalPathPoint`, `Marker`, `PathPoint`, `PhysicalOffsetFrame`, or `Station`
  component via the `Add` menu would crash the tab with a frame-related error message (thanks @AdrianHendrik, #800)
- Internal: the codebase now contains shape-fitting algorithms for fitting a sphere, plane, or ellipsoid analytic geometry
  to mesh data. The algorithms were written to closely match the shape-fitting codebase that came with "P.Bishop et. al,
  How to Build a Dinosaur, doi:10.1017/pab.2020.46" (#798)


## [0.5.3] - 2023/10/06

0.5.3 adds support for persisting 3D viewport options between boots of OSC, which is handy if you have preferred visualization
options that you typically always toggle on/off. It also adds support for importing stations into the mesh importer from a
CSV (File > Import Stations from CSV in the mesh importer workflow), which is useful for importing (parts of) a muscle path
into a mesh importer scene.

- 3D viewport settings (e.g. muscle styling, show/hide the floor, background color) are now persisted between
  boots of OSC via a per-user configuration file (thanks @tgeijten, #782)
- The right-click step size menu for scalar property editors now includes suggestions for masses (it assumes
  the property is expressed in kilograms)
- Added `Import Stations from CSV` to the mesh importer's file menu, which provides a way of importing stations
  into the mesh importer scene from a CSV file (thanks @emmccain-uva, #149)
- Hovering over a recent or example osim file in the home tab now displays the full name of the file in a
  tooltop (thanks @AdrianHendrik, #784)
- Editors for component `double` properties (almost all scalar properties) now have `-` and `+` stepping
  buttons (previously: the steppers were only available for `Vec3` properties; thanks @tgeijten, #781)
- The `Show`, `Show Only This`, and `Hide` buttons in the component context menu are now disabled if the
  clicked component, and all its children, have no `Appearance` property (#738)
- Internal: `osc::Material` now supports `osc::CullFace` (related: #487)
- Internal: The LearnOpenGL part of `oscar` is now compiled as a separate project that uses `oscar` as a library
- Internal: The demo/experimental tabs in `oscar` are now in a separate project (`oscar_demos`)
- Internal: Both `oscar` and `OpenSimCreator` are now compiled with `/W4` warnings enabled in MSVC, some
  warnings are manually disabled in `OpenSimCreator` until upstream patches are applied
- Internal: Refactored `OpenSimCreator` to use `oscar` as-if it were any other 3rd-party API (previously: it was
  included in its own block of includes as-if it were special)
- Internal: All of OpenSimCreator's UI-related code (e.g. tabs, widgets, panels) was refactored into `OpenSimCreator/UI`
- Internal: Added `osc::MandelbrotTab` to `oscar_demos`


## [0.5.2] - 2023/09/12

0.5.2 adds a few nice-to-haves when editing components' positions w.r.t. other frames, or resocketing them. It
also addresses a few crashing bugs that were introduced during `0.5.1`'s quite aggressive refactor (sorry about
those).

### Features

- The `Reassign Socket` popup now has a `Re-express $COMPONENT in chosen frame` checkbox, which users can
  tick to prompt OSC to automatically recalculate what the location/position/orientation properties of the
  component should be in the new frame, in the current scene state, such that it does not move or rotate. This
  is handy for (e.g.) re-socketing muscle path points onto different bodies, changing joint topologies, etc. (#326)
- Property editors for `Station`s, `PathPoint`s, and `PhysicalOffsetFrame`s now also display a user-editable
  `(parent frame)` dropdown, which lets users select which frame the `location`/`translation` properties
  of those components are edited in (#723)
- Sliders in the coordinate editor now look much more like "typical" sliders, and are now entirely
  disabled when locked (#36)
- Added the `Calculate` menu from the frame definition workflow to the OpenSim model editor and simulator
  workflows workflow, which means that users can calculate the position/orientation of a frame/point in an
  OpenSim model w.r.t. some other frame by right-clicking the element in the scene (#722)
- Added a settings editor next to the `Convert to OpenSim Model` button in the mesh importer that contains
  an `Export Stations as Markers` option. This lets a user export any stations in the mesh importer scene
  as markers instead, so that they can see their muscle points in the official OpenSim GUI (#172)
- Added an `Export` submenu when right-clicking a mesh in the mesh importer, which lets users re-export meshes
  w.r.t. another element in the mesh importer scene (#169)
- Added right-click context menu support to the Navigator when running a simulation (#776)
- Adding a new path point to a muscle using the `Add Path Point` menu option now selects the newly
  added path point (#779)
- The `osc.toml` configuration file now supports setting `log_level`, which lets users change the log
  level before the application has booted (handy for debugging OpenGL, etc.; thanks @AdrianHendrik, #764)
- The reassign socket popup is now sorted alphabetically and shows the absolute path to the component when
  moused over, to make it easier to use when resocketing in large models containing duplicate component
  names (#769)
- The "Socket" menu in the component context menu now explains each column in the displayed sockets table
- The border between windows/panels/bars in the UI were made brighter so that it's easier to see when one
  panel ends and another begins
- Added a clarifying `note` in Tutorial 1's documentation that asks users to try and use the `Brick` analytic
  geometry, rather than a mesh file like `brick.vtp` (#175)
- OpenSim was upgraded from version v4.4 to v4.4.1 (#721)
- All other libraries were updated to their latest version (#721)

### Bugfixes

- Fixed a crash that can occur when showing component tooltips in the `Add` menu (#773)
- Fixed the muscle plot panel crashing when importing a CSV overlay (#755)
- Fixed deleting a `range` element of an `OpenSim::Coordinate` no longer causes the editor tab to crash (#654)
- Fixed the UI appearing very dark on earlier Intel GPUs that require explicit initialization of an
  sRGB screen framebuffer (specifically, Intel HD 530s; thanks @AdrianHendrik, #764)
- Fixed the up, down, and trash buttons sometimes not working when selecting path points for a
  new muscle, and relevant up/down buttons are now disabled (e.g. you can't move 'up' from the
  top row of the point list, #778)
- Fixed saving a file that is suffixed with the file extension without the dot (e.g. `somecsv`) now adds
  the extension onto the end of the path (e.g. `somecsv.csv`, previously: would save it as `somecsv`, #771)
- Fixed a bug where, when selecting sockets when adding a new component, if two components in the model
  have identical names (e.g. `torso_offset`) but different locations (e.g. `/jointset/back/torso_offset`
  vs. `/jointset/acromial_r/torso_offset`) then only one option would be shown and it would be non-selectable.
  It will now show both options as `torso_offset` but, on mousing over, show the user the full absolute
  path to the component (#767)
- Fixed the scene scale factor applying to explicitly added `OpenSim::Geometry` in the scene (#461)
- Fixed The 3D camera will no longer move when dragging a visualizer panel around (#739)

### Internal Changes

- Internal: LearnOpenGL's physically based rendering (PBR) examples were implemented using OSC's rendering API
- Internal: `osc::MeshTopology` now supports `TriangleStrip`
- Internal: `osc::Mesh::set/getColors` now takes a sequence of `osc::Color`, rather than `osc::Rgba32`
- Internal: Added `osc::StandardTabBase`, to reduce code duplication in tab implementations
- Internal: Added `osc::ParentPtr<T>`, for parent pointer lifetime management
- Internal: The rendering API now supports rendering into cubemap render textures (#671)
- Internal: Shaders in the `resources/` dir were reorganized according to purpose
- Internal: `osc::WarpingTab` was renamed to `osc::MeshWarpingTab` (#725)
- Internal: The frame definition tab now supports the `Performance` panel (#733)
- Internal: Refactored many OpenSim methods to use RAII-/type-safe equivalents (#756, #758)
- Internal: Refactored OpenSim-style `int T::getSize` with stdlib-style `size_t size(T const&);` (#757)
- Internal: Refactored `osc::OutputSubfield` to be more type-safe (#759)
- Internal: Refactored `osc::ComponentRegistry` into a cleaner set of components (#760)
- Internal: Refactored various widget constructors to have similar signatures (#761)
- Internal: Refactored logging API into separate files
- Internal: The simulator tab 3D viewer was refactored to use the generic `osc::Popup` API (#776)


## [0.5.1] - 2023/07/27

0.5.1 adds an "Export Points" tool to the osim editor and support for non-participating landmarks in the
warping UI.

The exporter enables users to export any point-like quantity in an OpenSim model (e.g. muscle
points, stations, markers, and frame origins) with respect to a user-chosen frame as a CSV file.

The non-participating landmark feature enables users to import point data from a CSV file as
landmarks that hitch a ride through the warping kernel without participating in how the warping
kernel is computed. The resulting warped points can then be re-exported as a CSV file.

- Added "Export Points" tool, which lets users export points in a model w.r.t. some frame as a CSV (#742)
- Added support for non-participating landmarks in the mesh warper (#741)
- Fixed Undo/Redo hotkeys not working in the warping workflow tab (#744)
- The upper input size limit on many text inputs was removed


## [0.5.0] - 2023/07/11

0.5.0 adds support for "workflows", and implements two of them: frame definition and mesh warping.

The frame definition workflow makes it easier to define locations/rotations/frames on meshes according
to ISB-style relationships. For example, it has support for defining landmarks, midpoints, edges between
points, cross product edges, and frames that are defined w.r.t. those points/edges. This enables users
to visually place frames on bone scans according to ISB rules with visual feedback and on-the-fly
calculations, which should be useful to model builders.

The mesh warping workflow introduces a UI for the Thin Plate Spline (TPS) algorithm. The UI asks users
to pair landmarks from two separate meshes in order to create a TPS warping kernel. The warping kernel
can be then be used to warp any point in 3D space. We anticipate that this will be a useful tool for
performing morphology-driven model scaling and creating blended models.

The "workflows" feature, including the frame definition and mesh warping workflows, are work in progress.
If you think something is missing, or broken, report it on the issues page. We anticipate that interesting
features from each specific workflow (e.g. the `calculate` feature in the frame definition UI) will gradually
be ported to existing UIs (e.g. the model editor).

Change list:

- Fixed keyboard keyboard camera controls (e.g. arrow keys) should no longer control the camera if the
  user is interacting with some other UI element (e.g. a text input box, thanks @itbellix, #679)
- The "Simulate Against All Integrators" button in the tools menu now better-aligns with other buttons
  in the same menu (#684)
- Experimental model-editing-related tabs now appear in an `Experimental Tools` submenu in `Tools`, and
  an experimental excitation editor has been added that has a launch button in this new menu (#685)
- The `Appearance` property editor only updates the model after the user lets go of their mouse (#702)
  - This is to improve the performance of scrolling through colors
  - And prevents color edits from filling the undo/redo buffer of the UI
- Fixed live muscle plot sometimes glitches when moving the mouse over it with particular
  plots (thanks @JuliaVanBeesel, #703)
- Added support for editing `int` property values, which is handy for some component types (e.g.
  `PrescribedController`, #704)
- Added an "Export to OpenSim" button to the frame definition UI (#713)
- The splash tab now contains a "Workflows" section, which shows users the available specialized
  workflow tabs (#720)
- Selected elements (e.g. when selecting a frame in an add component dialog) are now easier to
  differentiate from not-selected elements in the UI (thanks @aseth, #677)
- Internal: OSC's renderer can now render all not-PBR parts of the LearnOpenGL tutorial series, which
  required adding support for multiple render targets (MRTs, #493)
- Internal: OpenSimCreator's source code is now split into "OpenSimCreator" and "oscar" (#635):
  - OpenSimCreator: code that is specifically for the OSC UI, and might depend on OpenSim
  - Oscar: OpenSim-independent engine code for creating a scientific UI
- Internal: The "oscar" (engine) part of OSC is now compiled with /W4 on windows (#686)
- Internal: `osc::PanelManager` now takes a `numInitiallyOpenedPanels` argument when registering spawnable
  panels (#692)
- Internal: `osc::PanelManager`'s lifetime hook methods (e.g. `onMount`) now follow the same naming
  conventions as `osc::Tab` (#691)
- Internal: `osc::ModelViewerEditorPanel` can now be constructed with a custom `onComponentRightClicked`
  callback, so that external code (e.g. in a frame editor, #490), can render custom context menus (#694)
- Internal: The software packaging scripts no longer have an `OpenSimCreator` prefix (#698)


## [0.4.1] - 2023/04/13

0.4.1 is a patch release that fixes some bugs that were found in 0.4.0. The only noteworthy user-facing
feature is that `GeometryPath`s can now be edited, giving users a new way to define muscles and other
path-based geometry (e.g. `PathSpring`).

0.4.1 also includes a variety of internal changes that make building and testing OSC easier. If you are
a user, these aren't important to you, but they make developers feel fuzzy inside.

- Fixed a segfault that would occur when adding a body with an invalid name. It now throws an exception and prints
  an error to the log instead (thanks @AdrianHendrik, #642)
- Fixed the manipulation gizmo rotation operation being broken when rotating scene elements (mostly, when rotating
  PhysicalOffsetFrames - thanks @AdrianHendrik, #642)
- Fixed "step by" popup radian buttons always change the step size to degrees, even when editing a quantity that is
  expressed in radians (thanks @AdrianHendrik, #643)
- Added basic support for editing `GeometryPath` properties (i.e. moving/deleting/adding/editing points in paths), so
  that the GUI supports adding `Ligament`s and `PathSpring`s (#645, #522, #518, #30)
- Because `GeometryPath`s can now be edited when adding new components (#522), `Blankevoort1991Ligament` was been
  removed from the component blacklist (previously: there was no way to add it because the user had no way of
  creating a bare-minimum geometry path, #518)
- Fixed any active popups/modals closing when changing between tabs in the main UI (#448)
- Fixed deleting a backing `osim` file while editing it via the model editor no longer causes the editor tab to close
  with a "file not found" error message (thanks @JuliaVanBeesel, #495)
- Fixed path points for non-muscular components that use `GeometryPath` (e.g. `PathSpring`) are now
  clickable in the 3D viewport (#647)
- Clicking a `PathSpring`'s path in the 3D viewport now selects the `PathSpring` rather than the underlying
  `GeometryPath` (#650)
- Removed OSC_DEFAULT_USE_MULTI_VIEWPORT as a build option (it is available as a runtime config option, #444)
- Fixed initial default panel positioning in "Simulate Against All Integrators" (#630)
- The build scripts (e.g. `scripts/build_debian-buster.sh`) now build single-threaded by default, because
  the OpenSim build step can exhaust a computer's RAM (#659)
- Disabled being able to delete joints from a model: it can cause segfaults that crash the UI (#665)
- Internal: Fixed osc::Material::getFloat/Vec3Array returning undefined memory (unused in user-facing code, #656)
- Internal: Renamed `src/OpenSimBindings/Renderering` to `src/OpenSimBindings/Graphics`
- Internal: Renamed `osc::Popups` to `osc::PopupManager`
- Internal: Added tests that ensure add each component type to a blank model (#298)
- Internal: Refactored test source files to match the source tree (#652)
- Internal: Refactored benchmark source files to match the source tree (#653)
- Internal: Switched `robin-hood-hashing` for `ankerl::unordered_dense` (#651)
- Internal: The renderer's unit tests now pass on MacOS (previously: had a shader-compiler error, #657)
- Internal: Dropped `fmt` library: collides with OpenSim and does not compile on modern GCC (#660)
- Internal: Cubemap rendering support was added to the graphics backend (#599)
- Internal: Ajay Seth (@aseth1) is now correctly listed as the final (PI) author (#668)
- Internal: Added automated test that ensures OSC can load all example osims with no issues (#661)
- Internal: Added automated test that ensures OSC can export all example osims to DAE with no issues (#662)
- Internal: Added basic tests for DAE formatter (#662)
- Internal: Added automated test that ensures OSC can render muscles as defined by a GeometryPath's color property (#663)
- Internal: Added automated test that ensures OSC can (try to) delete any component from RajagopalModel without issue (#665)


## [0.4.0] - 2023/03/06

- Added 'Lines of Action (effective)' and 'Lines of Action (anatomical)' rendering options to the model viewer
  (#562). They can be seperately toggled for muscle insertion/origin (#577). Clicking a line of action selects
  the associated muscle (#571). Thanks to @modenaxe for open-sourcing https://github.com/modenaxe/MuscleForceDirection
- `Station`s (#85), `PathPoint`s (#85), `PhysicalOffsetFrame`s (#583), `WrapObject`s (#588), and
  `ContactGeometry` (#596) can now be translated (or rotated, #584) in a model editor viewport with a
  clickable gizmo
- The editor and simulator tabs now have toolbars at the top containing common actions (e.g. undo/redo,
  save, load, scrub the simulation, etc.)
- OSC's logo was updated to make it visually distinct from OpenSim, and new icons have been added for toggling
  frames, markers, contact geometry, and wrap surfaces
- The model editor's 3D viewport now has the same keybinds as the mesh importer tab (e.g. press G to grab, R to
  rotate, left-arrow to orbit the camera, etc.; #44)
- The "Reload [Model]" button now forces OSC to also reload any associated mesh files, which is handy when
  seperately editing mesh files in something like Maya/Blender (thanks @JuliaVanBeesel, #594)
- Pressing the F5 key now performs the "Reload Model" action (#595)
- The simulation tab' time scrubber is now in the top toolbar of the simulator tab, and includes controls for
  stepping between states and changing playback speed (#563, #556). It supports a negative playback speed (#619)
- The `import CSV overlay` feature in the muscle plot panel can now import multiple curves from a single file (#587)
- There is now an (extremely EXPERIMENTAL) option to visualize plane contact forces in the 3D visualizer, using
  an algorithm similar to what's used in tgeijten/SCONE (#449)
- Shadow rendering is now default-enabled (#489). The scene light's slope/height no longer tracks along with
  the camera, which makes the shadow angle look a little bit better (thanks @tgeijten, #549)
- The camera now auto-focuses on the centerpoint of the model's bounding sphere when first loaded (previously:
  would focus on ground, #550) and it also auto-zooms the scene when first loaded (previously: it would try
  to auto-zoom, but it was a looser fit, #551)
- The colored alignment axes in the bottom-left of the viewport now also show the opposite direction
  of each axis in a slightly-faded color, to make them appear more symmetrical (#554)
- All mesh geometry that is used by example models is now centralized in the `geometry/` folder in
  OSC's resource directory (previously: some models had their own `Geometry/` dir, #560)
- Muscle coloring/rendering options now use generic terms like "Fibers & Tendons", rather than
  OpenSim/SCONE specific terms (#557)
- Added "Show All" to the "Display" menu of a component (previously: would only be available if the
  right-clicks nothing, #527)
- Added a "Show/Hide all of TYPE" option to the "Display" menu of a component in the editor (#528)
- The status (e.g. Running/Completed/Error) of the underlying simulator is now presented in the
  simulator UI's toolbar (#538)
- The UI's font size was reduced from 16 px (height) to 15 px (#570)
- Added basic documentation for the `ConstantDistanceConstraint` component (#534)
- Simplified the ruler tool so that it only shows the distance between two points (previously: would
  show a tooltip describing the second point, but that tooltip could overlap with the measurement, #31)
- Fixed inconsistent joint naming in tutorial 2, such that all joints are named `parent_to_child` (previously:
  `foot_to_ground` was in error, #546)
- Body center of masses are now an opt-in visualization option with a checkbox (previously: would show on hover,
  #573)
- Rendering `OpenSim::PointToPointSpring`s is how a user-facing toggle (previously: was always on, #576)
- The direction that the editor's 3D scene light is now right-to-left, to more closely match OpenSim GUI (#590)
- The camera control hotkeys (e.g. for zooming in, looking along an axis) are now documented in the button's tooltips (#620)
- There is now a toggle that affects whether the dragging gizmos operate in world space (ground) or the parent frame (#584)
- Added muscle coloring style "OpenSim (Appearance Property)", which uses the muscle colors as-defined in the osim
  file (previously: would always use OpenSim's state-dependent coloring method, which is based on activation, #586)
- The simulator panel will now only render the UI once one simulation state has been emitted from the simulator (#589)
- The spherical end-caps of muscle geometry now align better with the muscle cylinders they are attached to (#593)
- "Toggle Frame Visibility" (model-wide) now appears as an option when right-clicking joints, so users can more
  easily know it's an option (#50)
- Fixed a bug in the DAE exporter that caused it to throw an 'argument not found' exception (#581)
- The simulation parameters editor popup now has enough precision to modify the simulation step
  size, which is in standard units (seconds), but typically has very small values (nanoseconds, #553)
- Properties that have a name that begins with "socket_" now no longer appear in property editors
  when adding/editing a component in the model. Users should use the socket UI (right-click menu) to
  edit component sockets instead (#542)
- Fixed the splash tab showing out-of-date recent file entries when files are opened within other tabs (#618)
- If a simulation fails to start, the error now be shown in the toolbar and a panel will pop up explaining that
  the simulator tab is "waiting for first simulation report, open the log for more details" (#623)
- Cylinders generated by OSC now have correct surface normals, which is handy when exporting the model to
  (e.g.) a DAE file (#626)
- Fixed a memory leak in that would occur as the undo/redo buffer fills (i.e. over a longer editing session,
  #566)
- Hotfixed a bug from OpenSim where inertia edits were not applied to the model (#597, related: opensim-core/#3395,
  thanks to @jesse-gilmer for spotting this :))
- Attempting to open a file/folder (e.g. `osim` or `open parent directory of osim`) via OSC in Ubuntu no longer
  causes a crash if `xdg-open` is unavailable (e.g. on WSL2, #627)
- The (new) OSC logo now appears in the documentation pages (#615)
- Internal: renamed `osc::MeshTopography` to `osc::MeshTopology` (#544)
- Internal: the editor and simulation tabs are now widget-ized, so that new panels can be added more easily (#565)
- Internal: reorganized panel widgets from `Widgets/` to `Panels/` to (#564)
- Internal: OpenSim-related rendering code is now centralized in `src/OpenSimBindings/Rendering` (#572)


## [0.3.2] - 2023/01/09

0.3.2 is mostly a patch release with some minor quality-of-life improvements. It also includes all changes
from 0.3.1 (unreleased, because it contained a bug that was spotted last-minute). See `CHANGELOG.md` for a
full list of changes in both 0.3.2 and 0.3.1.

- The add component dialogs (e.g. `Add ClutchedPathSpring`) now print any addition errors inside
  the dialog (previously, would print the errors to the log, #476)
- The 3D scene now has basic support for rendering shadows (#10), but it must be enabled via a
  checkbox in a viewport's `Options` menu (it will be default-enabled in 0.4.0)
- The lightning bolt icon in the properties panel now also opens the context menu  when right
  clicked (previously: only left-click, #537)
- The navigator panel now only shows an expansion arrow icon for a top-level components (e.g. `ground`)
  if that top-level component contains children (#536)
- The "Edit Simulation Settings" popup now has a "Cancel" button (#535)
- Clicking a path actuator's (e.g. `ClutchedPathSpring`'s) geometry path in the 3D viewer will now select
  the actuator (previously: would select the geometry path, #519)
- An error message will now appear in the log if the GUI fails to delete a component from a model
  in the case where OpenSim Creator doesn't have a specialized deletion function (#531)
- The cancel button in the "Save Changes?" dialog now works in the mesh importer (#530)
- Opening a new mesh importer from the mesh importer tab (e.g. with `Ctrl+N`) auto-focuses the new tab (#529)
- Clicking frame geometry in the 3D scene will now select the frame geometry's parent (e.g. body, offset frame), rather
  than the geometry (#506)
- Error messages that appear in the "Add Component" popup should now span both columns (#514)
- The various 'subject01' example files are now prefixed with a relevant prefix (#513)
- All tutorial example files now have frame geometry toggled on by default (#505)
- Clicking, or tabbing out, of the name editor box in the properties editor panel now saves the name changes (#541)
- Fixed clicking out of a property editor box to an earlier property no longer clears any user edits (#516)
- Fixed a crash when viewing high-vertex-count meshes on Dell systems with an Intel Iris Xe GPU (#418)
- Fixed a segfault when adding a CMC component by hiding the CMC component from the add component menu (#526)
- Fixed a segfault from adding a `OpenSim::SpringGeneralizedForce` without a `coordinate` (#524)
- Fixed a segfault from adding a `OpenSim::CoordinateCouplerConstraint` without a `coupled_coordinates_function` (#515)
- Fixed a segfault from adding a `OpenSim::ActivationCoordinateActuator` without a `coordinate` (#517)
- Fixed a segfault from adding a `OpenSim::ExpressionBasedPointToPointForce` without a `body1/2` (#520)
- Fixed a segfault from adding a `OpenSim::PointToPointActuator` without a `bodya/b` (#523)
- Fixed a segfault from adding a new joint with 'ground' as a child (#543)
- Fixed the navigator panel having a slight background tint (#474)
- Fixed dragging an `osim` file into the editor tab should open the osim in a new editor tab (previously: did
  nothing, #501)
- Fixed the scene scale factor being ignored when using PCSA-derived muscle sizing (#511)
- Added `SimTK::`DecorativeCircle rendering support to the backend (#181)
- Added `SimTK::DecorativeMesh` rendering support to the backend (#183)
- Added `SimTK::DecorativeTorus` rendering support to the backend (#184)
- Refactored the `osc::Camera` API (#478)
- Refactored mesh class codebase-wide to leverage its copy-on-write behavior (#483)
- Refactored tab classes to have an `id()` function (#480)
- Refactored `unique_ptr` usage out of `osc::ShaderCache` (#496)
- Deleted duplicate `double_pendulum.osim` example file (#512)
- Deleted `Blankevoort1991Ligament` from the addition menu (it requires a GeometryPath editor to work, #518 #522)
- Deprecated (e.g. `Delp1990Muscle_Deprecated`), base (e.g. `PathActuator`), or illogical (e.g. `Ground`) components
  no longer appear in the `Add` menu (#512)


## [0.3.1] - UNRELEASED

**Note**: `0.3.1` was tagged in the git repository but never released, because it contained a bug that
caused it to crash on certain systems when viewing high-vertex-count meshes (e.g. CT scans).

- The mouseover hittest was re-optimized to match 0.2's performance (#349)
- Scene rendering performance has been improved by around ~10-30 % on typical machines
- The performance panel can now be opened in the mesh importer tab (#465)
- Vec3 property editors are now color-coded (#459)
- Added runtime bounds-checking to mesh indices (#460)
- Socket reassignment failure now fully rolls back the model to a pre-assignment state (previously: would erroneously hold some changes, #382)
- Hotfixed assigning a joint's `child_frame` socket to `/ground` to now cause an error message,
  rather than entirely crashing the application (#389 and opensim-org/opensim-core #3299)


## [0.3.0] - 2022/09/14

Live muscle plotting, output watching, better+faster graphics, and many other bugfixes/improvements.

### Summary

This is a general summary that contains *some* of the changes in 0.3.0. Read the "Raw Changelist" for an unsorted list
of all of the changes.

- NEW: "Muscle Plotting" (right-click a muscle, "Plot vs. Coordinate") is now greatly improved. It live-plots a muscle
  curve whenever edits are made, saves previous curves, enables users to lock previous curves, and revert to previous
  model versions, along with support for CSV import/export (#352 #346 #354 #355 #353 #365 #364 #363 #366 #362 #361
  #367 #359 #370 #368 #371 #230 #231 #394 #409 #398 #395).

- NEW: "Output Watches" panel in the editor. You can now right-click anything in the 3D editor, hierarchy, etc.
  and watch any associated OpenSim output of that thing (e.g. muscle stiffness, model kinetic energy, etc.).
  Editing the model automatically updates the output watch's value (#356).

- The 3D rendering backend was entirely rewritten. In general, 3D renders should look better, and have better
  performance. Please report any funky behavior/crashes to the GitHub issues page.

- The backend was upgraded from OpenSim 4.3 to OpenSim 4.4. In general, it is a little bit faster and is more
  stable in multithreaded contexts (#330)

- You can now show/hide model components more easily in the editor. The behavior is similar to OpenSim GUI (#415).

- The mesh importer has more translation options (e.g. "translate to mass center", #88, #92).

- There is now basic support for exporting any 3D viewport's content to a `.dae` file, for rendering the scene in external
  software, such as Blender (#314)

- The new rendererer is more resillient to meshes that that have inconsistent triangle windings or normals (#318)

- The mouse is handled more consistently in the editor screen's panels. Left-click usually selects stuff. Right-click
  usually opens a context menu.

- Some (effectively) "crash the UI" buttons have been temporarily removed until the backend can handle the
  edge-cases better (#329 #331). This also applies to the mesh importer (#331)

- The "Add Component" menus in the editor have been put into the main menu and right-click context menu. They
  are now cleaner and contain fewer duplicated entries (#312)

- You can now visualize ScapulothoracicJoints by ticking a checkbox in a 3D viewport's "scene" menu (#334)

- Plus a bunch of other UX improvements, stability improvements, crash hardening, etc. changes (see CHANGELOG.md)


### Raw Changelist

- There is now basic support for exporting any 3D viewport's content to a `.dae` file (#314)

  - It's in the "Scene" menu of each 3D viewport in the editor/simulator

  - This is useful for re-rendering the scene in 3rd-party software, such as Blender

  - Support is very basic right now. It only supports exporting a single frame (whatever is
    in the viewport) and does not try to group the geometry in any meaningful way

- Scene rendering is now cached if nothing in a viewport changes (selection, camera position, etc.)

  - The scene geometry list was already cached, this is another cache layer that caches the rendered
    texture

  - This reduces passive CPU/GPU usage when viewing, but not interacting with, the UI

- Added performance counters (and a performance panel) to the editor tab (#315)

- Mesh backface culling is now disabled by default (#318)

  - It causes issues when loading mesh files from sources that do not treat
    their triangle winding correctly (i.e. anti-clockwise). Those meshes were
    drawing their inner-edge, which is super annoying when working with (e.g.)
    XRay scans etc.

- Mesh normals are now rendered in a double-sided (#318)

  - This causes shading to work correctly if the mesh file contains inverted
    normals.
  - This is related to backface culling, because OpenSim Creator uses the mesh
    winding order to determine the surface normal (cross product)

- The hierarchy viewer now uses left-click to select/navigate stuff, and no
  longer auto-opens the children of your selection unless you explicitly click
  the arrow (#323)

- Mac releases are now built using the `macos-11` container (#329)

- Removed the ability to add wrapping geometry via the "Add Component" menu (#333)

  - The reason this was removed is because it causes the UI to crash if you use it.

  - The reason it causes a UI crash is because wrap objects implicitly require custom
    code to call `setFrame`. If it isn't called, it can result in crashes

  - Later releases will include custom support for adding wrapping surfaces. This just
    (effectively) disables a "crash the UI" button

- Removed the ability to add `CustomJoint`s via the "Add Joints" menu (#331)

  - The reason it was removes is because it causes the UI to crash if you use it.

  - The reason it causes a UI crash is because `CustomJoint`s *require* that the
    coordinates are fully set up before putting it into the model. The UI's automated
    support for adding stuff (joints, forces, etc.) can't automatically handle these
    requirements right now.

  - Later releases will include support for creating (and visualizing) `CustomJoint`s
    in-UI. This just (effectively) disables a "crash the UI" button

  - Note: you can still *load* and edit models containing `CustomJoints`, and edit
    osim files with hot-reloading. This change only removes the `Add Custom Joint`
    button because OSC cannot automatically gather all the necessary information to
    ensure no crash happens.

- Fixed the mesh importer crashing when converting a mesh importer scene into an
  OpenSim model (#331)

  - Related to the above. 0.2.0 added more components types, including `CustomJoint`,
    which was automatically added to the mesh importer

  - The mesh importer *also* cannot automatically create a valid `CustomJoint` and was
    crashing when it had to convert mesh importer scenes, which don't use OpenSim into
    `OpenSim::Model`s (which do)

  - Again, support for `CustomJoint`s is on the roadmap: this just disables unintended
    functionality that was causing the UI to hard-crash

- Removed duplicated components in the "Add components" menu that already appear in the
  grouped ("Add Joints, Add Muscles") menus (#312)

- Upgraded OpenSim to v4.4 (#330)

- Added basic support for drawing the ellipsoid of ScapulothoracicJoints (#334)

  - It's currently exposed a checkbox under the "scene" menu

- There is now a 'translate to mesh bounds center' menu item in the mesh importer (#88)

- There is now a 'translate to mesh average center' menu item in the mesh importer:

  - It takes the average center by adding up all the vertices and dividing by the number
    of vertices

  - This can sometimes be a handy approximation of "mass center", when the mesh isn't a
    closed surface (a requirement, if you want to compute an actual mass center)

- (Experimental) There is now a 'translate to mesh mass center' menu item in the mesh importer (#92):

  - It uses "signed tetrahedron volumes" to do this, rather than something expensive like
    voxelization

  - This means that the resulting mass depends on the provided mesh being a closed surface
    with correct normals etc.

- Added a "Hidden" option for muscle decoration style, which enables hiding all muscles in the model (#338)

- Cylinders (e.g. those used in muscles) are now smooth-shaded, and cost less to render (#316)

- All experimental screens are now tabs, available from the + menu (#264)

- The coordinate editor should no longer jump around when editing the coordinates of highly constrained
  models (#345)

- Fixed being unable to edit the parameters of a `HuntCrossleyForce` (#348)

- Hovering/clicking the title of an output plot in the simulator tab now hovers/selects the associated
  component, if applicable (#347)

- Fixed live muscle plots not plotting if the muscle being plotted is locked (#352)

- Disabled being able to (unsucessfully) scrub a muscle output plot if the coordinate is locked (#346)

- Replaced all tabs with spaces in the source code (#357)

- EXPERIMENTAL: added support for watching outputs in the editor (#356)

  - It shows the value of a model output for the model's default (initial) state
  - Eventually this panel will contain support for searching and removing output plots
  - Long-term the plan is to support plotting any watched output against a model coordinate, to replace
    the live muscle plots feature with a more generic "live output plots" equivalent
  - Beware, it is quite slow right now (due to internal APIs)

- Forward declares in the source code now use the Include What You Use style (#283)

- Live muscle plots now plot the data as it's being computed, rather than showing a loading bar (#354)

- Live muscle plots now show the previous few plot lines, for easier comparison (#355)

- Muscle plotting (e.g. muscle missing) errors are now propagated to the muscle plot panel (#353)

- There is a now a "copy .osim path to clipboard" button in the edit menu of the model editor (#365)

- The plot line now has a little bit of padding along the Y axis, to prevent it touching the axis lines (#364)

- The plot line now annotates the current coordinate and hover Y values, along with showing where the datapoints are (#363)

- The legend in each muscle plot now lists kind of edits that were made for each line of the plot (#366)

- Previous muscle curves now constrast slightly more and can have their datapoint markers optionally enabled (#362)

- Previous muscle curves can now be deleted by right-clicking in the legend (#361)

- Previous muscle curves can now be locked by right-clicking them in the legend (#367)

- Muscle curves should no longer replot if the user edits the coordinate in the viewer (#359)

- The muscle curve history now displays "loaded $OSIM" rather than "initial commit" (#370)

- Previous muscle curves can now be reverted to by right-clicking them in the legend (#368)

- Locked muscle curves now have a blue tint (#371)

- Muscle curves can be imported externally from a CSV file (#230)

- A single muscle curve can be exported by right-clicking it in the legend (#231)

- All plotted lines in a single muscle plot can be exported to a sparse CSV file (#231)

- The mesh importer, when given multiple mesh files, will try to import as many as
  possible, emitting an error message to the log for each failure (previously: would
  refuse to import all mesh files if any in the batch were erroneous, #303)

- The "reassign socket" popup will now display error messages more clearly when a
  connection reassignment cannot be made (#332)

- The "reassign socket" popup will now only display sockets that the component can
  actually connect to (#381), and the popup is optimized to ensure that creating
  the candidate list is fast (#384)

- The property editor panel now shows the properties at the very top of the panel (#392)

- The editor tab now has a status bar, which shows breadcrumbs to the current selection (#403)

- The properties editor panel now only shows properties (previous: component hierarchy path,
  contextual actions, and properties)

  - The model hierarchy now shows in a status bar at the bottom of the editor
  - Contextual actions are now always available via the general context menu that appears
    whenever something is right-clicked
  - See #403

- Vec3 properties in the property editor panel now show the X, Y, and Z components on separate
  lines, with `-` and `+` buttons for stepping the associated value down/up. The step size can
  be adjusted by right-clicking the input (#401).

- Renamed 'isolate'/'clear isolation' to 'show only'/'show all'/'clear show only' (#393)

- The muscle plotting widget's "export CSV" now has a submenu that enables users to select
  which (or all) curves they want to export (#394)

- Updated ImPlot to e80e42e (#409) - contains perf improvements etc.

- You can now change the muscle, output, and coordinate of any muscle plot via the title (#397)

- Muscle plots now have a "duplicate plot" action, which creates a separate plot panel with
  the same coordinate and muscle, but it does not (yet) copy the chosen output or previous
  curves (#398)

- The editor screen's main menu now has an `Add` menu for adding components (#405)

- Editing a coordinate via a muscle plot no longer causes the edit to add a curve to the
  curve history list (#395)

- The coordinate editor panel now has a more space-efficient layout (#413)

- The splash screen now has an improved layout that fits better on smaller laptop screens (#41)

- Fixed the `is_visible` flag being ignored for `GeometryPath` components (#414)

- Replaced "isolated" and "showing only" functionality with OpenSim-GUI style actions (#415):

  - The contextual (right-click) menu of every component in the model now has a "Display"
    menu containing "Show", "Show Only", and "Hide" actions, which behave similarly to
    OpenSim GUI

  - This replaces the OSC-specific "isolate" system. If you want to clear your isolation,
    you can right-click on the model and use "Display -> Show" to show all components in
    the model (like OpenSim)
    
- The property editor now contains a little "actions" lightning button to open the component's
  context menu (#426)

- Fixed osc not opening the splash tab when opening files provided via the argument list (#427)

- User-defined muscle path points are now individually hoverable/selectable in 3D viewports.
  Selecting a user-defined muscle path point highlights it in the 3D viewport, making muscle
  adjustments a bit easier (#425)

- Fixed UI 3D viewers showing tooltips, regardless of hover state (#417)

- Right-clicking on nothing now shows a display menu with a "Show All" option (#422)

- `osc::SelectionEditorPanel` is now called `osc::PropertyEditorPanel` (#428)

- The "Hierarchy Panel" is now called "Navigator" to match OpenSim's existing nomenclature (#429)

- The "Coordinate Editor" panel is now called "Coordinates" to match OpenSim's existing nomenclature (#435)

- The "Properties Editor" panel is now called "Properties" (#436)

- Added the ability to take a screenshot of the entire UI by pressing F11 while in the main UI (#442)

- On Windows, fatal application errors now write a crash file to the user data directory
  before exiting (#441)

- The UI is now more resilient to exceptions. It will now try to show them inside the UI as
  an error tab, rather than completely crashing the application (#440)

- All documentation was updated to reflect the latest UI changes (#452)


## [0.2.0] - 2022/07/04

Tabbed interface support, alternate muscle visualization options, OpenSim 4.3 support,
live muscle plotting, simulation performance measurements, and a variety of other
improvements.

Main changes (includes some changes that were released between 0.1.0 and 0.2.0):

- OSC now has a tabbed interface (#42):

  - The home screen, editor screen, simulator screen, and mesh importer now
    open in separate closeable tabs

  - This makes it easier to (e.g.) edit a mesh importer scene while (e.g.)
    running a simulation, or editing a model

  - Making a new model, mesh importer, or simulation opens a new tab containing
    the result (previously: overwrites current screen)

  - A simulator tab (previously: *the* simulator screen) now shows exactly one
    simulation (previously: it would show one simulation, but present a list that
    the user can select from)

  - Naturally, doing this involved many other changes: #207, #208, #210, #211,
    #212, #213, #214, #216, #218, #219, #228, #221, #222, #224 (and more)

- Muscle tendons can now be visualized in the 3D viewer (#165, pre-released in 0.1.6)

  - It's in the "options" menu of each 3D visualizer. Each viewport can have different
    muscle visualization settings

  - Work in progress: tendon locations and appropriate buttons etc. for this feature will
    be added in an upcoming release

- EXPERIMENTAL support for live muscle plotting (#191, earlier iteration released in 0.1.6)

  - Right-click a muscle and "plot against" a coordinate

  - This shows a muscle output (e.g. moment arm) vs. the coordinate, and
    automatically recomputes the plot whenever the model is changed

- The backend is now using OpenSim v4.3 (#192, released in 0.1.6)

- Simulations now accurately report the "wall time" spent computing each simulation step
  and the total simulation time (#202)

  - These counters come directly from the simulator thread and are more accurate than
    using something like an external stopwatch.

- You can add a wider variety of components via the UI

  - Some components may fail to add. This is usually because the UI doesn't support
    editing a property of the to-be-added component that the OpenSim backend requires. The
    UI still supports *trying* to add it, though.

- Coordinate edits are now part of the model (#242)

  - Editing a coordinate in the editor saves that edit to undo/redo for easier exploration

  - Any coordinate edits are directly saved to the model. This makes it *much* easier to
    create a default pose for the model (the most likely use-case, when building a model).

- A wider variety of STO files can be loaded via the `Load Motions...` feature (#284)

Other changes (and more details of the above):

- Fixed a multithreading bug where the simulator would occasionally crash with "Device or resource busy" (#201)
- Simplified the tooltip that's shown when hovering something in the model editor 3D viewer
- Fixed "New Model" action not creating a new model
- Right-clicking a 3D viewer in the model editor now shows "add stuff" context menu (#203, #215)
- Clicking "Open Documentation" in the splash screen should no longer crash the UI (#205, #209)
- The Mac build now builds with fewer dependencies (internal, related: #206)
- Hotfixed the first point in a moment arm muscle plot is always wrong (#220)
- Added tabbed-UI testing screen (internal)
- Exceptions that propagate to `main` are now unhandled (#227, internal)
  - This makes debugging the exception easier, because the stacktrace will originate from the `throw`
- Added an integrator performance analysis tab (#226)
  - This is an advanced feature that has been added to the `tools` menu in the editor tab
  - This makes it easier to compare the performance of `SimTK::Integrator`s for a particular sim
- Simulations now cancel faster, and hung simulations should be closeable in more situations (#244)
- Right-clicking a component in the simulations tab now shows a context menu that enables users to
  select which output(s) they want to plot (#240)
- There is now a "Reload osim" button in the model editor's edit menu, which manually reloads the osim
  from scratch (#247)
  - osc already automatically reloads osim files whenever the underlying file changes. This feature is
    mostly useful for updating the editor when an un-tracked file changes (e.g. a third-party resource
    that is indirectly loaded via a component in the model, such as meshes)
- Right-clicking an output plot in the simulation selection details panel now shows a "Request Output"
  button that can be pressed to add the output to the list of monitored outputs (#241)
- Right-clicking any output plot in the simulation tab now shows an appropriate button for removing the
  output from the user's output watch list (#250)
- Renamed "Request Outputs" to "Watch Output(s)" and used an eye icon to indicate design intent (#251)
- Tutorial 1 now contains links to download intermediate versions of the model (#62)
- Undo/redo now remembers coordinate edits made via the coordinate editor (#267)
  - And it remembers coordinate edits via a muscle plot panel
- Coordinate edits are now written to the `default_` properties of that coordinate (#242)
  - This replaces the other idea of having people press an extra button (#265)
- Fixed an application crash that could happen when changing a joint type (#263)
  - It was because the backend didn't clear cached pointers to the old joint (see issue)
- Rim highlighting looks a little bit better in high-aspect-ratio viewers (#273)
- The 3D scene light in the editor and simulation screens now points away from the camera more (#275)
  - This makes it easier to see inside of concave geometry
- The floor is now visible through semi-transparent geometry, such as wrapping surfaces (#276)
- The 3D scene now uses shader values that makes edges in the model "pop" more (#277)
  - This is intended to make it easier to view the model in the editor and simulation tabs
- The meshimporter wizard documentation page was removed (#262)
  - It was written before Tutorials 3 & 4 were written. Those tutorials explain it better
- Saving a model should no longer trigger a model reload (#278)
  - Previously, the system that monitors the on-disk file would notice it has been updated
    and dutifully reload the model.
- The 'challenge: model a finger' page was removed (#279)
  - It was hastily written in earlier versions of the tutorial documentation. The main tutorials
    supercede it
  - Challenges/exercises may be later re-introduced, but in a more fleshed-out form
- A wider variety of STO/MOT files can now be loaded (#284)
  - Previously, `osc` would only accept STO/MOT files that are in a modern (OpenSim 4.0+) format,
    with all states provided
  - It now accepts legacy (OpenSim <4.0) STO/MOT files - even if they have state variables missing
  - If a state variable is missing, a warning is printed to the log
  - That warning may be upgraded to a user prompt in later versions
  - The main utility of this feature is that it enables loading older STO/MOT files, and motion files
    that aren't *strictly* simulation states, for tertiary analysis by the user
- The simulation scrubber now correctly handles simulations that start at t != 0 (#304)
  - This is mostly for loading STO and MOT files
- The "Save All (to CSV)" button(s) in the output watches tab are now hidden if no outputs are
  export-able (#125)
- Reversed the keyboard camera panning direction in the mesh importer (#73)
- The experimental screen now has a "return to main UI" button (#307)
- Textual outputs can now be removed from the "output watches" panel  by right-clicking them (#306 #124)


## [0.1.6] - 2022/05/09

New muscle visualization options, experimental support for live muscle plots, and OpenSim 4.3 support:

- Muscle tendons can now be visualized in the 3D viewer (#165):

  - You can try this out by experimenting with the new dropdown menus that were added to the `Options` menu,
    available in each 3D viewer panel
  - Muscle decoration style can now be changed (e.g. to SCONE-style) in the 3D viewer's options menu (#188)
  - Muscle coloring logic can now be changed in the 3D viewer's options menu (#189)
  - Muscle thickness logic can now be changed (e.g. to PCSA-derived) in the 3D viewer's options menu (#190)

- (EXPERIMENTAL) There is now partial support for plotting muscle parameters (e.g. moment arm, pennation angle)
  against a particular coordinate (e.g. `knee_angle_r`) in the model editor (see #191):

  - The easiest way to do this is to right-click a muscle `Add Muscle Plot vs` and then select a coordinate (#196)
  - Alternatively, add a new plot panel via the `Window` menu in the editor's main menu
  - You can right-click a plot to change what it's plotting
  - You can left-click a plot to change the coordinate's value (#195)
  - The plot will auto-update whenever either the model is edited or a state is edited (e.g. by changing a
    coordinate value). It will also update if the model's backing file changes (e.g. because it is being
    edited in a third-party editor)
  - You can have multiple muscle plots open simultaneously (related: #191)
  - Plot data is computed on a background thread (#200)
  - See issue #191 on ComputationalBioMechanicsLab/opensim-creator for general progress on this feature

- You can now change how muscles are colored by the backend (#189):

  - Previously, muscles were always colored by the OpenSim backend (i.e. by activation)
  - You can now also color by activation, excitation, force, or fiber length
  - See the `Options` menu of a 3D viewer in the model editor for more information

- OpenSim was updated to v4.3 (#192)
- Fixed a bug that prevented model rollbacks from working correctly

- Internal changes (you should probably stop reading here):

  - The README docs have been updated to include a link to the YouTube tutorial videos
  - The README docs have been updated with clearer install/build instructions
  - Added an ImPlot demo screen to the experimental screens section
  - The new (work in progress) renderer backend has been fleshed out some more
  - The googletest feature is now actually being used by something (testing the renderer API)
  - The undo/redo/commit API has been exposed internally, which enables widgets to only draw
    new contents when the model has been updated (e.g. to recompute data plots)
  - The commit API is designed to have threadsafe methods, so that background threads can
    checkout/copy OpenSim models without interrupting the UI thread


## [0.1.5] - 2022/04/11

- Fixed thread race that causes visual glitches when running forward-dynamic simulations on models containing
  wrapping surfaces
- Fixed AABB/BVH visualization being broken in 3D model viewer panels
- Updated Mac OSX application icon to match other platforms
- Added googletest unit testing support (internal)


## [0.1.4] - 2022/04/07

- Fixed crash when renaming components


## [0.1.3] - 2022/04/05

- A simulation can now be loaded from an STO file:

  - E.g. save a simulation/motion in the official OpenSim GUI as an STO file
  - Open the model in OpenSim Creator
  - Drag the STO file into the OpenSim Creator model editor UI
  - OR use the "load motion" option in the main menu (#179)
  - Your STO motions should shown in the UI "as if" it were a "real" simulation
  - **Limitation**: STO files are always resampled to 100 Hz. This is because the official OpenSim
    GUI can export STO files containing extremely high-frequency samples, which `osc` can't yet
    handle

- You can now add a much wider range of components in the editor UI (#154):

  - This includes the ability to add controllers (#171)
  - And lists all force components (#154)

- Added a "Want to save changes?" prompt when closing the editor (#72)
- Simulation playback can now be paused/resumed (#16)
- All output plots, including meta plots (e.g. number of integration steps), are now
  scrubbable and can be saved as a CSV
- Switching between grab(G)/rotate(R)/scale(S) is now displayed as icons in the mesh
  importer (#65)
- Added `reorient 90 degrees` option in the mesh importer context menu (#160)
- Output plots are now rendered with `ImPlot`, which should make them easier to view
  and make them pop less while a simulation is running
- Hovering over a body in the UI now shows where the center of mass is as a black sphere (#60)
- Added option for switching between degrees/radians input when editing 'orientation' properties (#55)
- Renamed "Open" and "Save" in the mesh importer to "Import" and "Export" (#143)
- Fixed scale factors property being ignored for some types of OpenSim geometry in the osim editor (#141)
- Fixed an edge-case segfault that happened when editing the properties of a body that was synthesized
  by OpenSim to break a graph cycle (#156)
- The "Save All" button does not show if there are no simulation plots (partially fixes #125)
- Removed ability to uninstall OpenSim Creator from the installer (#131)
- Fixed out-of-bounds data access in application initialization that was detected by libASAN (be5d15)
- Refactored decoration generation backend to use osc::Transform instead of raw matrices (internal)
- Improved performance where the model decorations were being generated twice during undo/redo storage (internal)
- Improved general state/model editing performance (internal)
- Coordinates in the coordinate editor panel now show in OpenSim storage, rather than alphabetical, order (#155)
- Refactored a variety of internal APIs (internal)
- Partially integrated experimental DAG implementation (internal)
- Refactored undo, redo, and deletion logic to be more reliable (internal)
- Removed 'declareDeathOf' API: underlying implementation is hardened against this (internal)
- Moved FileChangePoller from OpenSimBindings/ to Utils/ (internal)
- Added a 'perf' panel to the simulator screen for in-prod perf measurements (internal)
- Upgraded Windows build to Visual Studio 2022 (internal)
- `osc::Screen` implementations must now handle the QUIT event themselves (internal)
  - This is to support behaviors like "you have unsaved changes, want to save them?"
- Fixed edge-case thread race crash on simulation screen (internal)
- Fixed Y component of SimTK::DecorativeCone ignoring scale factors (internal)


## [0.1.2] - 2022/02/16

- Fixed a bug in the mesh importer where scaling a mesh element would cause its rotation to be
  broken in the imported OpenSim model (#153)
- Fixed minor typo: renamed 'scale' to 'Scale' in mesh importer right-click context menu (#129)


## [0.1.1] - 2022/02/11

- Fixed exception-throwing code sometimes crashing the UI completely instead of showing an errr message (#132)
- Made automatic error recovery pop the UI slightly less
- Fixed the name editor in the property editor not updating when the user selects something else (#133)


## [0.1.0] - 2022/02/09

- Added this CHANGELOG (#104)
- Fixed a bug where stations within a geometry path were double-rendered with the path points (#103)
- Added a tooltip and reset context menu to the coordinate editor (#98 #69)
- Fixed a variety of little typos/errors in the documentation
- Fixed importing an osim into the mesh importer was ignoring mesh scaling (#110)
- Fixed log panel in mesh importer not having a menu bar (#109)
- Fixed tutorial documentation specifying FreeJoint instead of PinJoint (#113)
- Fixed osim editor grid to be in 10cm increments (#117)
- Fixed osim editor chequered floor height offset to be scale-dependent to prevent depth fighting (#114)
- Made the ruler tool show distances with higher precision (#121)
- Made float inputs accurate to 6 d.p. for small model builders (#120)
- Fixed alignment axes overlay not showing in editor/simulator screens (#118)
- Fixed ImGuizmo turning off when the camera is very (millimeter-scale) zoomed in (#119)
- Made all file dialogs automatically add a file extension if the user doesn't add one (#111)
- Made undo/redo remember the user's selection state (#108 #112)
- Fixed (hackily) muscle wrapping being broken in the visualizer (#123)
- Added "windowed fullscreen" mode for nicer screen recording


## [0.0.8] - 2022/02/07

- A variety (>50) of little UX improvements have been made in the mesh importer. Examples: 
  - All scene elements support a variety of translation and reorientation options
  - The menus are uniformly arranged and easier to click
  - Editors should behave more uniformly and will save on TAB/Enter
  - The fonts should be softer now
  - And many more... :wink:
- Added tutorials 3 (build a pendulum in the mesh importer) and 4 (build a hand/finger in the mesh importer)
- Added support for **stations** in the mesh importer, which let you mark "points of interest" in the model
  - These are points within some frame
  - Stations can now be selected as muscle path points, enabling you to create muscle paths between stations
- Added support for defining multi-point paths when creating a muscle
  - Previously, you could select multiple points, but couldn't rearrange their order
  - Previously, you *had* to select a physical frame. Now, there's support for attaching muscle points to other types of elements, such as stations.
- You can now open/save the mesh importer screen's state
  - It will save the data as an "exported" osim file. The mesh importer can now "import" osim files. Don't try and import not-exported osims: the mesh importer works with an *extremely* simplified data model.
- There is now a (hacky) joint re-zeroing button in the osim editor UI
  - This will rotate a joint's parent frame to overlap with the child frame's current transformation, which sets a new "zero point" for the joint.
  - The utility of this is that you can transform a joint in the coordinate editor and then "re zero" the joint to whatever you transformed it to.
- Connection lines are now only shown when hovering something in the mesh importer
  - The connection lines became very dense on complex models
- The mesh importer and mesh editor screens now track file save state:
  - If a file is opened in OpenSim creator, it should now show in the window's title bar
  - Any edits to the file should show it as "unsaved changes", which is represented with the standard trailing asterisk (e.g. `Untitled.osim*`)
- You can now assign multiple meshes to a body in one step in the mesh importer by first selecting all the meshes, then pressing `A`, then clicking the body (previously: it would only assign one mesh)
- All mentions of the original adamkewley repo were removed
  - This is because the repo's official home is https://github.com/ComputationalBiomechanicsLab/opensim-creator
- Bodies that are added via the mesh importer now have nonzero inertia. Instead, they now automatically have an inertia vector that is 1 % of the body's mass in a (3D) diagonal direction
- Modal screens in the mesh importer now have a cancel button
  - This is in addition to the previous UX, which shown a header that explains pressing ESC exits the screen
- Coordinates in the coordinate editor are now in degrees, rather than radians
- Fixed a crash that could occur when exporting an OpenSim model from the mesh importer that contained invalid characters in a component name


## [0.0.7] - 2021/11/26

- Fixed crash that happened when deleting bodies
  - The implementation was failing to delete all joints attached to the deleted body. This resulted in the model containing invalid joints that join to the now-deleted body.
- Fixed a crash that happened when editing a model in the model editor
  - The bug effectively made model editing extremely unusable. It was happening because one part of the system (e.g. one that adds things into the model when you press something like `Add Body`) was failing to update the underlying system.
- The mesh importer's main menu now has a "new" button, which resets the scene
- Alignment axes are now shown in the bottom-right of the mesh importer's viewer window
- Camera controls, such as zoom in, zoom out, auto-size, and reset are now shown in the bottom-right of the mesh importer's viewer window
- The overlay buttons now try to emphasize importing meshes by having "Add Meshes" be a standalone button in the top-left of the button row
- Pressing S enables scaling mode
- The floor in the mesh importer is now a transparent grid
  - This makes it easier to view imported meshes that happen to fall below the floor line
- Importing mesh(es) now automatically selects them
  - Previously, they would be added into the scene but the user's selection would be left untouched.
- Bodies are now visually distinct from joints in the mesh importer:
  - Bodies appear as cubes with little cones on each face to indicate X, Y, and Z
  - Joints appear as tetrahedrons with each leg appropriately colored for X, Y, and Z
- Improved + unified context menu layouts;
  - The first section displays the name + type of the object, with an optional help tooltip containing more information
  - The next section contains user-editable properties for the object (e.g. translation)
  - The final section contains actions
  - The order of properties/actions in a section is roughly the same for each type
- Improved context menu keyboard/numeric inputs:
  - Pressing ESC while a context menu is open closes the context menu
  - Pressing TAB while inputting into a context menu tabs to the next input **and** immediately saves (undoable) the change
  - Presssing ENTER while inputting into a context menu saves any changes and closes the context menu
  - Context menu scalar inputs are now accurate to 6 decimal places (previously: 3)
- Importing a mesh from the mesh importer to the main OpenSim Creator editor UI now automatically rescales the scene camera to show the entire imported scene
- `Ctrl+8` now auto-focuses/zooms a scene camera (solidworks keybind)
- Coordinate sliders are slightly more slider-ey
  - Work in progress: they aren't slider-ey enough to be obvious to a typical end-user
- Added a Log panel to the mesh importer (useful for debugging)
- Panels (History, Hierarchy, Log) are now enable-/disable-able in the mesh importer
- The History panel is now disabled by default
- Connection lines are a little bit thinner
- The hierarchy viewer has been improved. It now contains clearer headers for each object type (mesh, body, etc.) with some documentation about each type
- **Dark UI**: Recolored the mesh importer. It now has a dark color scheme:
  - This is because it's cool with the kids
  - But also because it makes light (i.e. most scene elements) elements pop out against the background
- Mouse zooming is now faster
- You can zoom without a scroll wheel with `Alt+Right-Click+Drag`
- Transitioning between states (main importer, select something to create a joint, etc.) now places the new state in the same rectangle as the 3D viewer
  - This prevents the UI from "popping" when switching states: the rest of the UI now just fades a little bit and the new state is presented
- An animation now plays when entering the "create joint" state, which "pops" each possible joint target
  - This is because I was in the mood for some delicious easing functions
  - But also because it makes the joint-selection flow, which hurls the user into a selection state, a little more obvious


## [0.0.6] - 2021/11/12

This release mostly focuses on the new-and-improved mesh importer wizard, which can be accessed from the `File` menu in the UI.

The mesh importer wizard is a new, **still in development**, feature that enables users to add meshes, bodies, and joints into a 3D scene. Every element in the scene uses absolute (ground-based) coordinates, which lets users freely translate, rotate, and scale the scene elements without having to worry about (eventual) OpenSim model topography.

The "free form" scene can then be automatically converted into an OpenSim model, and this conversion process handles transforming the bodies, joints, and meshes into OpenSim's coordinate system. Our aim with this feature is to make it easier for researchers to get their mesh data into the OpenSim ecosystem, so that they can then (e.g.) start running simulations, adding muscles, adding contact geometries, etc.

Changelist:

- Frames are now clearer in the mesh importer
- Gizmo handles are now clearer in the mesh importer
- You can no longer annihilate the universe by pressing delete while the context menu is open in the mesh importer
  - This was an event-forwarding bug. The delete key deletes scene elements, the context menu should prevent this event from bubbling to the action, though, and it wasn't. The context menu is also showing the thing that's being deleted, so that causes a problem.
- In the mesh importer, Joints can now be aligned along two mesh points. Right click a joint center, change orientation, mesh points
  - This feature is still a bit buggy but was left in for 0.0.6 because I figured it would be better to integrate + show the buggy version during UX testing than leave it out completely.
- In the mesh importer, joint center frame axis lengths now scale based on the degrees of freedom in that joint. E.g. a pin joint will have a slightly longer Z axis.
- The colors, visualization, and interactivity of scene elements are now user-editable, so that users can recolor things to make them stand out, make them transparent, etc.
- There is now some basic documentation of the mesh importer in the documentation pages. This needs more fleshing out.
- There is now a basic "make a finger" challenge in the documentation pages.
- OpenSim creator will always try to render a couple of frames after transitioning to a new screen, to try and ensure that the screen has had a few ticks before running in event-driven mode. This deals with any rendering issues where the render was dependent on inter-frame values (e.g. deltas, previous frame dimensions, etc.)

## [meshimporter-1] - 2021/11/05

Custom build + release of latest mesh importer changes.

- Manipulation axes should no longer flip when they are reoriented
  - This was an ImGuizmo feature that, when you're looking at a gizmo from other angles, would draw (e.g.) the X axis dotted to indicate -X
- Manipulation gizmos and overlay lines should now always draw below other screen elements. Previously, the gizmo would draw over context menus
- Keyboard shortcuts should be slightly more predictable now. Sometimes, pressing a key wouldn't trigger the corresponding action because of how events are pumped through the UI
- The mesh importer screen will now "remember" its state between loads
  - This enables converting the importer scene into an OpenSim model and then returning to the importer scene (e.g. because something wasn't quite right)
- Dragging the camera around with your left/right mouse no longer deselects things
  - This was an unusual edge-case interaction between the hittesting step, which treats "clicked on nothing" as "deselect everything" and the camera's drag state machine, which typically also involves a click (also on nothing)
- Frames are now bigger, which should help them stand out from the manipulation gizmos and make them easier to click
- You can now no longer click on the mesh that's being assigned
  - Previously, this would un-assign the mesh, but caused problems when a user's bodies were inside the mesh
- **Reworked joint addition flow**:
  - Previous: user right-clicks body, clicks "join to", user is presented with a new state to select a body to join to, user is presented with a joint choice state, user is presented with a "place pivot point" state, user confirms the addition of the joint.
  - New: user-right clicks body, clicks "join to", user is presented with a new state to select the body to join to, joint is immediately added into the normal scene as a FreeJoint that's placed in-between the two attachment points
  - This flow is typically less confusing, and enables the user to leverage undo/redo while moving the pivot around (the previous pivot placement state did not support this)
  - Orientation controls were added to the joint's context menu to support (e.g.) orienting the joint center toward the parent/child
- **Meshes are now grouped with their attached bodies**:
  - Attaching a mesh to a body makes that mesh participate in a selection group that includes the body and all other meshes attached to the body.
  - This means that selecting the mesh, or the body, or another mesh in the group, will select every element in the group
  - This behavior can be overridden by holding down ALT while clicking a body
  - This behavior is ignored when right-clicking the mesh (it will show the context menu for the right-clicked mesh)
- There is now a basic hierarchy viewer, which makes clicking intercalated elements easier.


## [0.0.5] - 2021/09/29

This release contains a wide variety of general UX improvements which were identified during a model-building hackathon. It also changes the installation location of OpenSim Creator in Windows to ensure that newer versions overwrite older ones.

---

**Important**: The default installation location of OpenSim Creator has changed. 

In general, the new location(s) do not contain a version number and installing newer versions of OpenSim Creator will overwrite the older versions. The reason for doing this was to prevent the confusion caused by allowing multiple versions to be installed concurrently, which would create multiple start menu shortcuts, etc. and make it difficult to know whether the latest version is being used after an upgrade.

- e.g. **Windows**. Old location = `C:\Program Files\osc ${version}`. New location = `C:\Program Files\OpenSimCreator\`

You should manually uninstall older versions of OpenSim Creator to ensure that you are always running the latest version.

---

Changelog:

- 3D viewer panels now have a close button
- 3D viewers now have zoom in and zoom out buttons under `scene`, to help users that can't scroll easily
- Setting the mass in the `add body` popup now correctly sets the mass of the added body
- Plotting an output subfield (e.g. `angular_velocity.x`) now correctly displays it in the output plots and output CSV export
- The `about` tab now contains a shortcut to open OpenSim Creator's installation directory
- The `view` tab now contains a shortcut to open the currently-open `osim`'s file directory. Handy for viewing adjacent files, like STOs and Geometry.
- All output plots now contain a vertical line indicating where, in time, the simulation is currently scrubbed to
- Output plots can now be clicked to scrub the simulation to that point in time
- Output plots now have a horizontal X axis at `Y=0.00` if the data ever crosses that boundary
- The mini axis lines in the bottom-left of a 3D viewer are now bigger and labelled with X, Y, and Z
- The action buttons now have an extra `+` symbol to make them slightly more visible
- Contact geometry can now be removed from a `HuntCrossleyForce` in the UI
- The `Selection View` panel is now hidden when editing a model: the information is redundant and appears in the `Edit Properties` panel anyway
- 3D viewers now have a `Measure` button, for measuring 3D distances in the model. Click it, click where you want to start the measurement, drag to where you'd like to measure.
- In addition to reassigning sockets in the `Edit Properties` panel, you can now browse to them (i.e. make them the selected component) by right-clicking them
- Assigning cone geometry to a body now correctly displays cone, rather than cylinder, geometry
- Fixed a bug where menu padding could break sometimes, causing all the UI elements to bunch up to the edge of UI panels
- The `add_body` modal now automatically focuses the body's name when initially opened
- Dragging an `.osim` file into the splash screen now loads the `osim` file in the editor


## [0.0.4] - 2021/09/22

This is the latest release of OSC. Depending on the platform, the installer may install OSC alongside previous versions. If this isn't the behavior you want (e.g. because it causes multiple OSCs to appear in your start bar) then uninstall the previous version (e.g. run `Uninstall.exe` in `C:\Program Files\osc` on Windows). Later versions of OSC may be packaged to do that automatically.

This contains a variety of fixes/improvements over 0.0.3, including:

- A coordinate editor in the main editor screen
- Limited support for ultra-large/ultra-small models, such as fly legs, by scaling the scene
- The coordinate editor and hierarchy panels now have a search bar
- The mouse controls are now a union of OpenSim's and Blenders, which should make OSC easier to use in a variety of circumstances (especially laptop touch pads)
- The scene light now moves with the camera, so the model has no dark spots
- (work in progress) In-built documentation. Currently contains two introductory tutorials. These will be improved over time.
- More built-in analytical geometries are supported, such as arrows and cones
- The rendering pipeline is ~20-30 % faster on lower-end devices (e.g. laptops with iGPUs)
- The UI renders *much* less frequently if a user isn't interacting with it. Typically reduces resource usage by 50-70x
- Rim highlights should now always show correctly, and not occasionally be occluded by other things in the scene (this was particularly annoying when rendering overlapping frames)
- The camera can now zoom in very far, provided the camera's focal point is set correctly. This enables zooming into (e.g. very small features)
- The camera has more keybinds (subject to change): `X`, `Ctrl+X`, `Z`, `Y` move the camera along `+X`, `-X`, `Z`, and `Y` axes respectively (`Ctrl+Z` and `Ctrl+Y` are already taken by `Undo` and `Redo`). `F` resets the camera. `Ctrl+F` automatically tries to move the camera to view the whole screne
- The locale now defaults to `C`, rather than whatever the system is. This handles the edge-case of saving a file while using a locale that has decimal numbers like `3,14`. OpenSim will save the file with `3,14` but then fail to read the number in when the file is loaded later, substituting in `3`.


## [0.0.3] - 2021/07/14

Contains a wide variety of improvements over 0.0.2:

- OSMV (the codename for the alpha) has now internally been renamed to OSC, or OpenSim Creator. The official repository is now at https://github.com/ComputationalBiomechanicsLab/opensim-creator
  - This is mostly a branding change, but also renames the C++ namespaces from `osmv` to `osc`
  - OSC uses the OpenSim logo, rather than the custom one that was used in earlier versions
- All releases of the software now use OpenSim 4.2
  - This adds support for things like ScapulothoracicJoint
  - It also makes simulations slightly faster (typically, 20-40 %)
  - Mac users *may* (needs verification) need to install `gfortran`: please report installation issues on fresh mac products
- OpenSim's log now prints to the OSC log panel
  - This means any warnings, error messages, etc. from OpenSim can be seen in the UI
- Component default values are now improved
  - These are the property values components have when added through the UI
  - This is an ongoing process. Some components (e.g. ContactSphere) still have poor defaults (e.g. radius=0.0)
- Fixed a bug where keybinds could be triggered, even when typing something into an input textbox
  - This could occasionally lead to annoying situations. E.g. typing `R` (reload) into an input box would reload the model
- Modal dialogs no longer trigger popups/changes in UI elements below the dialog
  - E.g. mousing over something in a modal should no longer trigger tooltips, 3D scene changes, etc. below the modal
- Simulation states are now saved
  - This means that you can retrospectively view the entire simulation with an animation scrubber
  - It also means you can retrospectively plot outputs from a previously-ran simulation
  - The reporting interval is deterministic, with a fixed time step, so that data+animations are consistent (e.g. datapoints in plots will be evenly spaced)
  - Simulations make their own independent copy of a model before running. This means that you can keep editing a model while a simulation runs on older versions of the model.
- Simulations are now more configurable
  - This means that users can change the integrator, minimum step size, maximum step size, etc. of a simulation
- Multiple simulations can now be ran concurrently
  - The UI provides a way for selecting which simulation is currently being viewed
  - There is currently no limit, or queuing, on the parallelism - try not to let your computer explode
- Simulation metadata--eg. the number of integration steps, error rates,  etc.--is now displayed on the simulation screen with basic documentation explaining each plot
- Any plotted outputs can be directly exported to a CSV file
  - Choose to plot an output, view a simulation, go to the "outputs" tab: there's now a "Export to CSV" button
- The UI is now uses custom style colors, which are darker and less blue than the ImGui defaults
- The UI now uses FontAwesome for icon support. FontAwesome icons have been added to the most commonly used actions in the UI
- The component hierarchy viewer now supports expanding and contracting nodes
- Switching between editing and simulation mode no longer wipes the undo/redo history of the edited model
- The "About" menu has been tidied up and now contains more information + documentation.
- The "Experiments" screen (accessed via the About menu) now prompts the user to select an experimental feature
  - This is because we plan on using this space to ship experimental features to production early.
- Added "Windows" tab to the main menu
  - Lets users enable/disable UI panels
  - Multiple 3D viewers can be enabled through this
  - Currently does not save which panels were enabled/disabled between UI boots (a future version will support this)
- The splash screen now contains obvious-looking "New" and "Open" buttons right above the main content
  - This is so that new users don't have to search around in the "File" tab of the main menu
- Example filenames are now sorted case-insensitively, so that they appear to be logically alphabetical (e.g. A, a, B b vs. A B a b)
- 3D viewers now use Blender-like keybindings
  - e.g. Left/Right click interact with things. Middle mouse for swiveling, CTRL+middle mouse to pan, etc.
  - This is because it's fairly common for model builders to also be using blender
- You can now add muscles to a model through the UI
  - Requires selecting N>1 bodies/frames in the model
  - Currently, there is no support for rearranging the attachment order
- You can now add geometry to a body/frame
  - Previously, you could *assign* geometry to a body/frame, which would overwrite any previous assignments
  - Adding enables adding multiple pieces of geometry to a single body/frame
  - You can delete geometry by pressing DEL while geometry is selected
- Added support for attaching analytical geometry (cubes, spheres, etc.)
  - The benefit of using these is that the resulting model file is less dependent on external mesh files (the geometry is generated in-code by OSC)
- The loading screen now has a progress bar
  - The progress bar always marches forward and is no indication of actual progress: purely there to show the UI is doing *something*
- **Experimental**: Added mesh importer wizard
  - Enables importing multiple mesh files into a 3D scene without having to first build an OpenSim model
  - Users can then rearrange the meshes in 3D space and assign bodies/joints to them while moving things around in a global, Blender-like, coordinate system
  - Once the user has set everything up in this wizard, it automatically generates an OpenSim model from the assignments
  - This is work in progress. It requires further development, but may still save a significant amount of time over manually building the model step-by-step through the main editor UI
- **Experimental**: You can now plot subfields of an output, rather than only being able to plot `double` value types
  - Practically, this means that you can plot a `Vec3`'s `x`, `y`, or `z` coordinates
  - Experimental, because the output names etc. need to be fixed when exporting the resulting (subfield) output to a CSV file


## [0.0.2] - 2021/04/12

Contains a wide variety of fixes/changes from 0.0.1: far too many to list here (as osmv stabilizes, the release notes will start rolling out).


## [0.0.1] - 2021/01/06

If you are in any way sane, you wouldn't use this.
