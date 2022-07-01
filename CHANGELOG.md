# ChangeLog

All notable changes to this project will be documented here. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Unreleased]

- The UI is now a tabbed interface (#42):

  - The home screen, editor screen, simulator screen, and mesh importer now
    open in seperate closeable tabs

  - This makes it easier to (e.g.) edit a mesh importer scene while (e.g.)
    running a simulation, or editing a model

  - Making a new model, mesh importer, or simulation opens a new tab containing
    the result (previously: overwrites current screen)

  - A simulator tab (previously: *the* simulator screen) now shows exactly one
    simulation (previously: it would show one simulation, but preset a list that
    the user can select from)

  - This is still work-in-progess. Fixed: #207, #208, #210, #211, #212, #213, #214,
    #216, #218, #219, #228. To-fix: #221, #222, #223, #224

- Fixed a multithreading bug where the simulator would occasionally crash with "Device or resource busy" (#201)
- The forward dynamic simulator now also reports "Wall Time" and "Step Wall Time" (#202)
  - This is useful for performance debugging
- Simplified the tooltip that's shown when hovering something in the model editor 3D viewer
- Fixed "New Model" action not creating a new model
- Right-clicking a 3D viewer in the model editor now shows "add stuff" context menu (#203, #215)
- Clicking "Open Documentation" in the splash screen should no longer crash the UI (#205, #209)
- The Mac build now builds with fewer dependencies (internal, related: #206)
- Hotfixed the first point in a moment arm muscle plot is always wrong (#220)
- Added tabbed-UI testing screen (internal)
- Exceptions that propagate to `main` are now unhandled (#227)
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
    that is indirectly loaded via a component in the model)
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
