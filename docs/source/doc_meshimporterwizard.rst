.. _doc_meshimporterwizard:

Mesh Importer Wizard
====================

This is a standalone documentation page for the Mesh Importer Wizard feature in OpenSim Creator:

.. figure:: _static/meshimporterwizard_screenshot.png
    :width: 60%

    The mesh importer UI. The importer is specifically designed to make importing meshes as free-form as possible. In the UI, you can freely move meshes, bodies, and joints before converting everything into a standard OpenSim model.

.. warning::

    🚧 This documentation is work in progress 🚧. The mesh importer wizard is a new feature in OpenSim Creator and is still being changed frequently. Because of that, this documentation might not be accurate.


Accessing
---------

The mesh importer UI can be accessed from the File menu in the splash screen or in the editor:

.. figure:: _static/meshimpoterwizard_mainmenubutton.png

    Mesh importer button location in the main menu.

This will replace whatever is open in the current screen with the mesh importer screen.


Key Features
------------

This is a kitchen-sink list mesh importer features that you might find useful, rather than an authoritative guide on how to use the importer. Experimenting with the importer yourself is the fastest way to explore the benefits/pitfalls of these features.

- All scene elements in the mesh importer can be freely moved, rotated, and (if manipulating a mesh) scaled. The mesh importer will converts all positions + orientations into OpenSim's relative coordinate system during the import phase.

- You can import meshes through the "Add" menu, or by right-clicking the scene and using the context menu, or by dragging + dropping mesh file(s) into the window.

- You can directly import and attach a mesh to a body by right-clicking a body and using the "attach mesh to this" item. The mesh will be loaded, reoriented to match the body, and attached to the body.

- You can create joints from bodies to other bodies (or ground) by right-clicking a body and clicking "join to".

- The joint type of any added joint can be edited by right-clicking the joint center.

- All scene elements are also accessible through the hierarchy panel. This is especially useful when the scene is very complex and contains a lot of overlapping elements.

- The visualization, locking, and color of elements in the scene can be customized. This is useful in more-complex scenes.

- The visualizer has a "scale factor" parameter that can be adjusted to make the frames and floor texture smaller/bigger. This is especially useful when working on extremely small/large models (e.g. insects).

- All scene elements can be safely deleted/renamed

- All operations have undo/redo support. Undo/redo is implemented as a labelled history. You can manually navigate to any entry in that history in the "History" panel.

- Reopening the mesh importer remembers the last state of the mesh importer. This means that you can import a model, see if it works (simulates well, is well-formed in OpenSim), and then go back and make further edits in the importer (if necessary).
