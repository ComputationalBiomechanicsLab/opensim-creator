---
title: 'OpenSim Creator: An Interactive User Interface for Building OpenSim Musculoskeletal Models'
tags:
  - opensim
  - musculoskeletal modeling
  - musculoskeletal mechanics
  - simulation and modeling
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

OpenSim Creator is an open-source integrated graphical interface that lowers the barrier to building and evaluating musculoskeletal models in OpenSim, which often required editing XML files or custom code. This paper describes the architecture and features of OpenSim Creator and demonstrates its application in developing new musculoskeletal models.


# Statement of Need

OpenSim [@Delp:2007; @Seth:2018] is an open-source musculoskeletal modeling platform with a large and growing user base across diverse fields, demonstrated by its use in clinical [@Marconi:2023], biomechanical [@Porsa:2016], robotics [@Belli:2023], and paleobiological [@Beesel:2021; @Demuth:2023] studies. However, creating a musculoskeletal model remains labor-intensive and technically demanding because it requires the careful integration of biological data, biomedical images, mesh editing tools, model/data file formats, and musculoskeletal anatomy.

For example, building a biomechanical model of a dinosaur involves 3D scanning fossils, segmenting/registering bones, identifying reference frames and muscle attachments, rigging/articulating skeletal structures, reconstructing musculature, and running biomechanical simulations [@Bishop:2021]. Such workflows often need to be repeated for different subjects or specimens. While scaling existing models can reduce this effort, developing robust and unbiased scaling procedures that handle diverse shapes, injuries, or deformities remains challenging [@vanDerKruk:2025]; therefore, real-time visual feedback during model building and scaling is a powerful feature for model creation, verification, and validation.

The lack of graphical tools not only affects research but also limits accessibility for students and new researchers learning musculoskeletal modeling. As OpenSim's use expands beyond biomechanics (e.g., medicine, paleobiology), and open education becomes more widespread (e.g., online and open courses [@Weller:2018]), there is a growing need for open-source, visual, and easy-to-install tools that educators can use to teach musculoskeletal model-building concepts clearly — tools that lower barriers to entry compared to traditional code- or biomechanics-focused workflows.


# State of the Field

Similar model creation and editing challenges are addressed in fields such as 3D design (e.g., Blender [@Blender]), engineering (e.g., FreeCAD [@FreeCAD]), and game development (e.g., Godot [@Godot]). Engineers commonly use these software tools to reduce labor by automating actions such as 3D object placement. They also enhance user expertise by enabling model editing in a safe, undoable environment with immediate visual feedback.

OpenSim includes a user interface (UI) that can edit the parameters of existing OpenSim models, but it is specialized around tools that accept existing model files as input. For example, it includes tools for scaling models, computing inverse kinematics (IK) from marker motion recordings, and computed muscle control (CMC, [@Thelen:2006]), but it does not include tools for adding bodies or joints to an existing model. Therefore, to create new models, users typically use a combination of the following three options:

- **Manually write the model file (XML)**: The majority of models used by the OpenSim community were created by the development team by manually editing OpenSim model files (.osim files, XML format). It is possible to build any model this way, but it requires knowledge of and expertise with OpenSim's data structures. For average end-users, it is highly error-prone because it does not provide immediate or visual feedback.
- **Use code/libraries to customize a model**: Examples include the MAP Client [@Besier:2014], Neuromusculoskeletal Modeling Pipeline [@Hammond:2025], and Muscle Parameter Optimizer [@Modenese:2016]. These are powerful tools to make existing models more representative of individuals, but they typically do not enable the creation of new models.
- **Use visual tools that can export OpenSim models**: Examples include NMSBuilder [@Valente:2017] and MuSkeMo [@Bijlert:2024]. These visual tools manipulate a separate data structure exported to OpenSim and do not support direct editing, viewing, or simulation in-UI.

There remains a need for an integrated UI that supports real-time model development, live simulation, and displays key quantities (e.g., muscle moment arms, force vectors) directly from the OpenSim API during editing without export or translation.


# Design Principles and Architecture

We surveyed open-source tools (e.g., @Blender) to identify common design and architectural patterns. We found that, in most cases, they focus on providing immediate visual feedback whenever a user performs an action and are continuously improved over many releases. Immediate feedback is typically achieved by integrating the UI directly with the underlying model/state, rather than through indirect mechanisms such as remote procedure calls (RPCs) or export steps. Based on these observations, and our prior experience with OpenSim, we decided that OpenSim Creator's architecture should focus on the following four key requirements:

- **Enable responsive editing performance**: supports interactive model editing and fast visual feedback.
- **Provide low-level UI control**: supports a highly customized user experience. Specialized tasks in biomechanical model building (e.g., landmark placement, mesh reorientation, mesh warping, real-time data visualization) may require low-level control over the rendering process.
- **Directly integrate with the OpenSim API**: provides on-demand access to the OpenSim component API. This enables a wide variety of features, such as the ability to add and edit components in the UI, or show biomechanical quantities (e.g., muscle lengths, moment arms) during editing/simulation.
- **Support iterative software and model development**: facilitates rapid prototyping and feedback from the research community.

To meet these requirements, OpenSim Creator draws inspiration from the games industry, which prioritizes rapid prototyping and real-time rendering. Architecturally, we combine C++ for direct and high-performance OpenSim API integration, a custom OpenGL renderer for real-time 3D editing, and Dear ImGui [@ImGui] for a customizable low-level 2D UI toolkit. Additionally, real model-building challenges from our own research heavily informed OpenSim Creator’s design and architecture, ensuring that practical workflows were supported.

Organizationally, we adopted an open iterative development methodology. All releases of OpenSim Creator have been available on GitHub under an Apache 2.0 license from the first month of development, with new releases every 1--2 months. We have continuously received and incorporated feedback from the research community over 36 release cycles. To further support adoption and educational use, OpenSim Creator includes fully documented tutorials written using Sphinx that guide users with no prior OpenSim experience through model construction and interactive visualization. These resources allow students and new researchers to quickly learn musculoskeletal modeling concepts, explore workflows hands-on, and leverage OpenSim Creator's real-time features.


# Features and Scope

\autoref{fig:1} and \autoref{fig:2} illustrate some of the interactive workflows in OpenSim Creator. While it is challenging to quantify every feature in this article, users benefit from the following capabilities:

- **Live-reload of the underlying .osim file**: OpenSim Creator automatically reloads the model file from disk whenever it detects changes. This enables seamless integration with existing workflows, including manual XML edits, script-based modifications, or edits performed in the OpenSim UI.
- **Live muscle plots**: OpenSim Creator can plot muscle quantities (e.g., moment arm, fiber length) that automatically update whenever the model is edited--either via OpenSim Creator or via external tools. Users can overlay curves from CSV files on these plots, making it easier to create models that match experimental measurements.
- **Output watches**: The OpenSim API provides an "outputs" abstraction, allowing each model component to declare quantities of interest (e.g., a body's center of mass, a joint's reaction force, etc.). OpenSim Creator displays these outputs in real-time during editing and simulation, providing essential insight into model behavior.

![Screenshots of OpenSim Creator's specialized workflows. a) The mesh importer workflow, which focuses on reorienting and rigging meshes in a free-form editor, b) the mesh warper workflow, which focuses on placing corresponding landmarks on meshes (left and middle) with real-time updates to a Thin-Plate Spline [@Bookstein:1989] warped mesh (right). \label{fig:1}](images/fig1.png)

These features accelerate the model-building process and illustrate the value of OpenSim Creator's software architecture: the direct connection between the OpenSim API and the UI ensures that user interactions immediately update visualizations and model outputs.

These interactive features rely on a highly responsive rendering engine. OpenSim Creator maintains real-time performance, typically running smoothly - even when manipulating complex models on modest hardware (e.g., @Rajagopal:2016 on an Intel Core i5 MacBook Air 2020). This responsiveness ensures that live muscle plots, output watches, and mesh manipulations remain smooth and immediate, allowing users to focus on model construction rather than waiting for visual updates. The tight coupling between the UI and the OpenSim API is key to achieving this performance, avoiding indirect refresh mechanisms such as export/reload cycles or RPCs.

OpenSim Creator is primarily intended for interactive model construction, visualization, and direct inspection of biomechanical quantities. Its UI enables instant feedback during editing, supporting exploration and rapid iteration. Once a model is constructed, users can use the .osim file with other tools or scripts in the OpenSim ecosystem for simulation, analysis, or integration with experimental data. OpenSim Creator facilitates hands-on model construction, while downstream analyses leverage the broader OpenSim ecosystem.

# Illustrative Use Cases

Subject-specific models are necessary because all individuals are unique and can have a wide variety of injuries and deformities. In one project, our goal was to create a subject-specific model of a human shoulder. Using OpenSim Creator's mesh importer UI (\autoref{fig:1}a), a previously published [@Seth:2019] generic shoulder model was used as a template by superimposing it over subject-specific CT scans. The UI enabled real-time, free-form placement and orientation of the subject-specific joints and bodies. Muscle volumes from subject-specific MRI data were then combined with moment arm data from the generic model to create a line of action for each muscle in the model. OpenSim Creator's model editor UI provided real-time visual feedback about how each muscle's moment arm was affected by the placement of attachment points and wrapping surfaces on the muscle's line of action within the model (\autoref{fig:2}). 

![A screenshot of OpenSim Creator's model editor UI, showing a subject-specific human shoulder model created within the editor. The model editor can host multiple panels, each of which is updated whenever the user edits the model. For example, the muscle plotter (right) can be used to show how a muscle's moment arm changes as a user edits the muscle in the 3D viewport (middle) or in the properties panel (left).\label{fig:2}](images/fig2.png)

Additional use cases that involve OpenSim Creator are available in the literature, such as creating mouse forelimb models [@Gilmer:2025], creating female-based lower-extremity models [@Gouka:2025], modeling the effect of foot deformities in children [@Heuvel:2023], modeling BMXes [@Aumerie:2024], modeling speed skates [@Devaraja:2024], and modeling sit-to-walk movements [@Heijerman:2023].


# Conclusions

Model creation is a complex, multistep, and iterative process that is best served by realistic, fast, and interactive visual feedback. By combining a rapid-prototyping architecture inspired by the games industry with a development approach that continuously incorporates feedback from the biomechanical research community, we designed and developed a graphical UI that enables the creation and evaluation of OpenSim models more quickly and easily.

OpenSim Creator’s architecture supports rendering multiple high-vertex-count bone models in real time on a MacBook Air (2020, Intel Core i5). Its customizable UI allows researchers to build models visually, and its tight integration with the OpenSim API provides immediate feedback on biomechanical quantities such as muscle moment arms. Anecdotally, master's students using OpenSim Creator in a musculoskeletal modeling course, as well as researchers from the broader community, have reported reduced time and effort to construct, edit, and evaluate models compared to traditional workflows, including XML editing or script-based approaches. These models included complex musculature, multiple joints, and subject-specific anatomical variation, demonstrating that OpenSim Creator can handle realistic research-level scenarios efficiently.

Beyond research, OpenSim Creator has proven valuable in educational settings. It has been used in student courses, supported by fully documented tutorials that guide users with no prior OpenSim experience through model construction and visualization. This hands-on approach teaches musculoskeletal modeling concepts while lowering barriers to entry, providing immediate visual feedback, and removing the need for prior coding or engineering expertise.

Looking forward, OpenSim Creator will continue to evolve, with plans to support specialized workflows such as visually-assisted model warping, real-time sensor visualization, and the import of diverse biomedical data. These developments, combined with ongoing improvements to the core user experience, will further streamline model creation from medical imaging and emerging anatomical data, making OpenSim Creator a versatile tool for both research and education.


# Acknowledgements

This project was sponsored by grant numbers 2020-218896 and 2022-252796 from the Chan Zuckerberg Initiative DAF, an advised fund of Silicon Valley Community Foundation. It was also sponsored by the Dutch Research Council, NWO XS: OCENW.XS21.4.161. The  sponsors' involvement was exclusively financial.

We thank DirkJan Veeger and Bart Kaptein for shoulder CT and MRI data for the human model.


# References

