---
title: 'OpenSim Creator: A Graphical User Interface for Building OpenSim Musculoskeletal Models'
tags:
  - opensim
  - musculoskeletal modeling
  - musculoskeletal mechanics
  - simulation and modelling
  - biomechanics
  - rigid body dynamics
  - user-interface tooling
  - imgui
  - C++
authors:
  - name: Adam Kewley
    orcid: 0000-0002-6505-5374
    corresponding: true
    affiliation: 1
  - name: Julia van Beesel
    orcid: 0000-0001-5457-0103
    affiliation: "1, 2, 3"
  - name: Ajay Seth
    orcid: 0000-0003-4217-1580
    affiliation: 1
affiliations:
 - name: Biomechanical Engineering, TU Delft, Delft, Netherlands
   index: 1
 - name: Department of Development and Regeneration, KU Leuven Campus Kulak, Kortrijk, Belgium
   index: 2
 - name: Dept. of Human Origins, Max Plank Institute for Evolutionary Anthropology, Leipzig, Germany
   index: 3
date: 27 February 2025
bibliography: paper.bib
---


# Summary

OpenSim Creator is an open-source graphical interface that lowers the barrier to building and evaluating musculoskeletal models in OpenSim, which previously required editing XML files or custom code. This paper describes its architecture and features. It also demonstrates its use in developing new biomechanical models.


# Statement of Need

OpenSim [@Delp:2007; @Seth:2018] is an open-source musculoskeletal modeling platform with a large and growing user base across diverse fields, including clinical [@Marconi:2023], biomechanical [@Porsa:2016], and palaeobiological [@Beesel:2021; @Demuth:2023] studies. However, creating a biomechanical model remains labor-intensive, requiring the careful integration of biological data, and technically demanding, requiring expertise in biomedical imaging, model/data file formats, and musculoskeletal anatomy.

For example, building a biomechanical model of a dinosaur involves segmenting bone morphology, rigging skeletal structures, reconstructing musculature, and running biomechanical simulations [@Bishop:2021]. Such workflows often need to be repeated for each subject, and while scaling existing models can reduce this effort, developing robust and unbiased procedures that handle diverse shapes, injuries, or deformities remains challenging [@vanDerKruk:2025]. Real-time visual tools can provide valuable feedback during model building, scaling, and validation.

These challenges not only affect research but also limit the accessibility of musculoskeletal modelling in education. As OpenSim use expands beyond engineering (e.g., medicine, palaeobiology) and open education becomes more widespread (e.g., online and open courses [@Weller:2018]), there is a growing need for open-source, visual, and easy-to-install tools that educators can use to teach musculoskeletal model-building concepts clearly — tools that lower barriers to entry compared to traditional code- or engineering-focused workflows.


# State of the Field

Similar modeling challenges are addressed in fields such as 3D design (e.g., Blender [@Blender]), engineering (e.g., FreeCAD [@FreeCAD]), and game development (e.g., Godot [@Godot]). Engineers commonly use these software tools to reduce labor by automating actions such as 3D object placement. They also enhance user expertise by enabling model editing in a safe, undoable environment with immediate visual feedback.

OpenSim includes a user interface (UI) that can edit existing OpenSim models, but it is specialized around providing tools that accept existing model files as input. For example, it includes tools for scaling models, computing inverse kinematics (IK) from marker motion recordings, and computed muscle control (CMC), but it does not provide tools for adding bodies or joints to an existing model. Therefore, to create new models, users typically use a combination of the following three options:

- **Manually write the model file (XML)**: It is possible to build any model manually, but it requires understanding OpenSim's XML datastructure, is laborious, and does not provide immediate or visual feedback.
- **Use code/libraries to generate or optimize a model**: Examples include the MAP Client [@Besier:2014] and the Neuromusculoskeletal Modeling Pipeline [@Hammond:2025]. These can greatly accelerate specific tasks but require coding knowledge and are not general-purpose model-building suites.
- **Use visual tools that can export OpenSim models**: Examples include NMSBuilder [@Valente:2017] and MuSkeMo [@Bijlert:2024]. Most visual tools manipulate a separate datastructure exported to OpenSim and do not support direct editing, viewing, or simulation in-UI.

Despite these approaches, the ecosystem lacks a comprehensive, integrated UI that supports real-time model development and displays key quantities (e.g., muscle moment arms, force vectors) directly from the OpenSim API during editing without requiring an export or translation step.


# Design Principles and Architecture

We surveyed open-source tools (e.g., @Blender) to identify common design and architectural patterns. We found that, in most cases, they focus on providing immediate visual feedback whenever a user performs an action and are continuously improved over many releases. Immediate feedback is typically achieved by integrating the UI directly with the underlying model/state, rather than through indirect mechanisms such as RPCs or export steps. Based on these observations, and our prior experience with OpenSim, we decided that OpenSim Creator's architecture should focus on the following four key attributes:

- **Enable responsive editing performance**: allows interactive model editing and fast visual feedback.
- **Enable low-level UI control**: supports a highly customized user experience. Specialized tasks in biomechanical model building (e.g., landmark placement, mesh reorientation, mesh warping, real-time data visualization) may require low-level control over the rendering process.
- **Enable direct OpenSim API integration**: provides on-demand access to biomechanical quantities (e.g., muscle lengths, moment arms) with minimal development or performance overhead.
- **Enable iterative software and model development**: facilitates rapid prototyping and feedback from the research community.

To facilitate those requirements, OpenSim Creator draws inspiration from the games industry, which emphasizes rapid prototyping and real-time rendering. Architecturally, it combines C++ for high-performance OpenSim API integration, a custom OpenGL renderer for real-time 3D editing, and Dear ImGui [@ImGui] for customizable low-level 2D UI.

Organizationally, we adopted an open iterative development methodology. All releases of OpenSim Creator have been available on GitHub with an Apache 2.0 license from the first month of development, with new releases every 1--2 months. We have continuously received and incorporated feedback from the research community over 36 release cycles. Additionally, real model-building challenges from our own research heavily informed OpenSim Creator’s design and architecture.

# Features and Scope

\autoref{fig:1} and \autoref{fig:2} illustrate some of the interactive workflows in OpenSim Creator. While it is challenging to quantify every feature in this article, users benefit from the following capabilities:

- **Live muscle plots**: OpenSim Creator can plot muscle quantities (e.g., moment arm, fiber length) that automatically update whenever the model is edited--either via OpenSim Creator or via external tools. Users can overlay curves from CSV files on these plots, making it easier to create models that match experimental measurements.
- **Output watches**: The OpenSim API provides an "outputs" abstraction, allowing each model component to declare quantities of interest (e.g., a body's center of mass position or a joint's reaction force). OpenSim Creator displays these outputs in real-time during editing and simulation, providing essential insight into model behavior.
- **Live-reload of the underlying .osim file**: OpenSim Creator automatically reloads the model file from disk whenever it detects changes. This enables seamless integration with existing workflows, including manual XML edits, script-based modifications, or edits performed in the official OpenSim UI.

These features accelerate the model-building process and illustrate the value of the software architecture: the direct connection between the OpenSim API and the UI ensures that user interactions immediately update visualizations and model outputs.

These interactive features rely on a highly responsive rendering engine. OpenSim Creator maintains real-time performance, typically updating the UI at greater than 60 frames per second, even when manipulating complex models on a mid-range laptop. This responsiveness ensures that live muscle plots, output watches, and mesh manipulations remain smooth and immediate, allowing users to focus on model construction rather than waiting for visual updates. The tight coupling between the UI and the OpenSim API is key to achieving this performance, avoiding indirect refresh mechanisms such as export/reload cycles or remote procedure calls.

OpenSim Creator is primarily intended for interactive model construction, visualization, and direct inspection of biomechanical quantities. Its high-performance UI enables real-time feedback during editing, supporting hands-on exploration and rapid iteration. Once a model is constructed, users can use the .osim file with other tools or scripts in the OpenSim ecosystem for simulation, analysis, or integration with experimental data. OpenSim Creator facilitates hands-on model construction, while downstream analyses leverage the broader OpenSim ecosystem.

![Screenshots of OpenSim Creator's specialized workflows. a) The mesh importer workflow, which focuses on reorienting and rigging meshes in a free-form editor, b) the mesh warper workflow, which focuses on placing corresponding landmarks on meshes (left and middle) with real-time updates to a Thin-Plate Spline [@Bookstein:1989] warped mesh (right). \label{fig:1}](images/fig1.png)


# Example Use-Cases

Subject-specific models are necessary because all individuals are unique and can have a wide variety of injuries and deformities. Our goal was to create a subject-specific model of a human shoulder. A previously published [@Seth:2019] generic shoulder model was used as a template by superimposing it over subject-specific CT scans in OpenSim Creator's mesh importer UI (\autoref{fig:1}a). The mesh importer UI enabled real-time, free-form placement and orientation of the subject-specific joints and bodies. The scene was automatically converted into an OpenSim model. Muscle volumes from subject-specific MRI data were combined with moment arm data from the generic model to create a line of action for each muscle in the model. The model editor UI provided real-time visual feedback about how each muscle's moment arm was affected by the placement of attachment points and wrapping surfaces on the line of action within the model (\autoref{fig:2}).

Another study evaluated how a given biomechanical model simulated a sit-to-stand movement. The predictive simulations proved to be computationally expensive, and OpenSim Creator was used to simplify the existing model by eliminating wrapping surfaces and leveraging quick visual feedback (both as 3D renders and 2D plots) to place muscle via points while preserving the moment-arms of the original model (@Heijerman:2023).

TODO: From cvhammond : "For reproducibility, of the two example use cases that you mention in the paper, the first one does not contain a reference to the results of the work to create a subject-specific model of the human shoulder."

![A screenshot of OpenSim Creator's OpenSim model editor UI, showing a subject-specific human shoulder model made using it. The model editor can host multiple panels, each of which are updated whenever the user edits the model. For example, the muscle plotter (right) can be used to show how a muscle's moment arm changes as a user edits the muscle in the 3D viewport (middle) or in the properties panel (left).\label{fig:2}](images/fig2.png)


# Conclusions

By combining a rapid-prototyping architecture inspired by the games industry with a development approach that continuously incorporates feedback from the biomechanical research community, we designed and developed a graphical UI that enables the creation and evaluation of OpenSim models more quickly and easily.

OpenSim Creator's design and architecture enables it to render multiple high-vertex-count bone models in real-time on a mid-range laptop. Its control over the UI enables customized workflows where researchers can create models visually, and its tight integration with OpenSim enables it to show moment arms in real time as model creation proceeds. Our users report significantly reduced amounts of time and effort to build, edit, and evaluate models when compared to existing approaches.

TODO: from modenaxe: "Claiming quick model creation is certainly a major achievement and appeal of a software a scope like OCC, but the sentence at line 94-96 in my opinion needs more evidence. [...] What were these users trying to do? How complex was the model?"

As software constantly evolves, we are looking forward and anticipate leveraging OpenSim Creator to create specialized workflows for visually-assisted model warping, real-time sensor visualization, and the import of other biomedical data, in addition to continuing to improve the day-to-day user experience of creating an OpenSim model from medical imaging and emerging sources of anatomical structure and material properties.

TODO: related to a suggestion by modenaxe: mention the educational achievements of the project (e.g. its use in a student course) to tie into the stated educational goals in the "Statement of Need".


# Acknowledgements

This project was sponsored by grant numbers 2020-218896 and 2022-252796 from the Chan Zuckerberg Initiative DAF, an advised fund of Silicon Valley Community Foundation. It was also sponsored by the Dutch Research Council, NWO XS: OCENW.XS21.4.161. All sponsors' involvement was exclusively financial.

We thank DirkJan Veeger and Bart Kaptein for shoulder CT and MRI data for the human model.


# References
