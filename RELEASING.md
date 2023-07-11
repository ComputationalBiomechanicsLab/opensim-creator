# Release Checklist

These are the typical steps required to finalize a release of OpenSim
Creator.

How comprehensively the QA/testing steps are done is entirely dependent
on how many changes were made since the last release (assuming an automated
test suite isn't available that covers the changes).

- [ ] Create an issue called something like `Release XX.xx.pp`
- [ ] Copy this checklist into it
- [ ] Bump OSC's version number in `CMakeLists.txt` (`project`)
- [ ] Clean-build OSC on Linux with:
  - `-DCMAKE_BUILD_TYPE=Debug` (incl. for Simbody+OpenSim)
  - `CC=clang CXX=clang++ CCFLAGS=-fsanitize=address CXXFLAGS=-fsanitize=address` (incl. for Simbody+OpenSim)
  - `-DOSC_FORCE_ASSERTS_ENABLED=ON`
  - `-DOSC_FORCE_UNDEFINE_NDEBUG=ON`
- [ ] Ensure test suite passes with debug+ASAN build
- [ ] Manually spot-check new changes with debug+ASAN build
- [ ] Go through manual QA process (list below)
  - How comprehensively you do this is subjective
- [ ] Fix all bugs/problems found during the above steps
- [ ] Update `CHANGELOG.md` sections such that the current `unreleased`
      section becomes `XX.xx.pp` and then add a new `unreleased` section
      above that
- [ ] Commit any fixes to CI and ensure CI passes
- [ ] Tag+push the commit as a release
- [ ] Download artifacts from the tagged commit CI build
- [ ] Unzip/rename any artifacts (see prev. releases)
- [ ] Create new release on github from the tagged commit
  - Upload all artifacts against it
  - Write a user-friendly version of CHANGELOG that explains changes
- [ ] Update Zenodo with the release (requires adamkewley's Zenodo login
      to publish the automatically generated draft)
- [ ] Update `codemeta.json` with latest Zenodo release details
- [ ] Update `CITATION.cff` with latest Zenodo release details
- [ ] Update `README.md` with latest Zenodo release details (CITING)

## Manual QA Checklist

All steps of this QA checklist do not need to be completed. Use your discretion
based on which changes have happened since the last release.

(yes, manually testing things sucks; and yes, I'm aware that computers can automate
 things. The act of making computers automate things is called *software development*,
 which takes a non-negligible amount of time, and requires tradeoffs).

### Top-level Checks

- [ ] A fresh install has reasonable-ish panel locations
- [ ] The tabbed UI doesn't screw with the panel locations
- [ ] All user-facing tutorial documentation can be carried out end-to-end without any confusion, surprises, or crashes
- [ ] Can edit the coordinates of an existing model without any issues
- [ ] Can add any component type into the model without any *obvious* issues (having incorrect default values etc. is permissable because that's a backend issue that needs to be manually patched later)
- [ ] All component actions (e.g. adding subcomponents, isolation, etc.) work as expected in the editor
- [ ] Can simulate any model with a forward-dynamic simulator. All example models simulate fine.
- [ ] The simulator can be stopped/cancelled by closing the simulation tab
- [ ] The number of simulation reports is as-expected (inclusive time range, one report at start, one report per report interval, one report at end)
- [ ] Can request to monitor model outputs, and they are then shown in the simulator tab
- [ ] Can play with the muscle plotting feature
- [ ] The 3D viewers have the option to customize how muscles are visualized/decorated. This works in both the simulator and editor screen
- [ ] All links in about, experimental screens, install dir, user dir, websites, etc. work
- [ ] The "select muscle" part of the add muscle plot feature says "no muscles" if used on a model with no muscles
- [ ] Any defaulted names (coord names, etc.) should be within expectations
- [ ] Adding new components should default any weird property names to something sane

### Mesh Importer Checks

- [ ] Can create a model in the mesh importer that contains cyclical joint graphs (e.g. robot model with multiple feet)
- [ ] The mesh importer scene always exports 1:1 to an OpenSim model without moving/rotating anything in an unusual way
- [ ] The mesh importer scene can always be exported into a mesh editor tab
- [ ] The mesh importer scene can be exported to an osim
- [ ] Exporting to an osim clears the dirty flag
- [ ] All user-visible keybinds in the mesh importer work as expected
- [ ] All user-visible buttons work as expected
- [ ] Scene scale factors (e.g. for tiny insect models) are exported to the model editor screen
- [ ] Visibility, interactivity, etc. buttons in the mesh importer all work as expected
- [ ] It's possible to open multiple mesh importers (tabbed interface)
- [ ] Doing anything in one mesh importer tab has no effect on the others. E.g. mouse interaction, keybinds, drag & drop should only affect the currently-open mesh importer tab

### Model Editor Checks

- [ ] All contextual actions work for common component types
- [ ] Deletion kind of works for things like muscles, etc.
- [ ] When deletion doesn't work, it prints an error to the log
- [ ] Can add almost any component type via the UI buttons
- [ ] Most components have a basic help tool-tip that explains what the component does
- [ ] All buttons in the main menu work as expected
- [ ] The coordinate editor works as expected
- [ ] All modals have a clear cancellation button that works
- [ ] Modal content is correct
- [ ] Can have multiple 3D viewports open. They operate completely independently of eachover (ignoring scale fixups)
- [ ] Can right-click anything in the 3D viewports to produce a context menu for that thing
- [ ] Can right-click anything in the hierarchy viewer to produce a context menu for that thing
- [ ] Can click the lightning icon in the properties editor to produce a context menu for the selected component
- [ ] Can right-click anything in the hierarchy path (status bar) to produce a context menu for that thing
- [ ] Can right-click any coordinate in the coordinate editor to produce a context menu for that coordinate
- [ ] Show/Show Only This/Hide works as expected (give or take)
- [ ] Can (optionally) draw Scapulothoracic Joints

### Simulator Checks

- [ ] The hierarchy panel works as expected
- [ ] The selection details panel works as expected
- [ ] The outputs listing in the selection details panel allows for CSV (+open) support
- [ ] The outputs listing also enables "watching" the output, which adds it to the outputs watch list
- [ ] The simulation details panel works as expected
- [ ] The walltime/step walltime values are within the expected range (i.e. perf hasn't tanked)
- [ ] Any plot in the simulation details panel can be exported to CSV (+opened?)
- [ ] All plots can be exported to a single CSV
- [ ] 3D viewports in the simulator tab shows similar options to in the editor screen (i.e. the ones that make sense)
- [ ] The simulator tab scrubber works mostly as expected (work in progress :wink:)
- [ ] An STO file can be imported from OpenSim 4.0, 4.1, 4.2, 4.3, and 4.4 (beta)
- [ ] The performance panel (advanced) works as expected
- [ ] The simulator panel doesn't cause adjacent panel resizing/closing to misbehave (because it not contains more custom UI widgets)
- [ ] All STO files from the `opensim-models` repository can be loaded and return reasonable-looking results

### Muscle Plotting Checks

- [ ] Can right-click any muscle in a variety of OpenSim models (e.g. from `opensim-models` repo). It shows a list of coordinates. Clicking the coordinate starts a muscle plot.
- [ ] Can open multiple muscle plots at once
- [ ] Editing a model causes a replot of all open plots (multiple allowed)
- [ ] Historical curves are saved for all open plots
- [ ] Can save any (or all) curve(s) to a CSV file
- [ ] Can import a curve from a CSV file (incl. an exported one)
- [ ] Can import a curve from a CSV file created in Microsoft Excel
- [ ] Can import a curve from a CSV file created from a python script
- [ ] Can change the muscle, coordinate, and value (e.g. MomentArm) of any plot. Changing them causes a replot, but holds onto any imported CSV files
- [ ] Can change the number of datapoints that are plotted
- [ ] Can scrub the coordinate via the plot by left-click + dragging over the plot
- [ ] Can open the context menu from the `options` button
- [ ] Can move/hide/move-out the plot legend
- [ ] Can clear previous curves. This should keep all locked + CSV curves, though.
- [ ] Can (un)lock any curve, incl. the active one
- [ ] The previous curves are annotated with a human-readable description of the edit that enacted the curve
- [ ] Can revert to a previous version of the model by right-clicking the curve in the legend
