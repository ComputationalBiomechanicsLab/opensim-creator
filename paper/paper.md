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
  - cad tooling
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

OpenSim Creator is an open-source "biomechanical CAD" user interface (UI) that focuses on building and evaluating musculoskeletal models in OpenSim, something that previously required manually editing extensible markup language (XML). In this paper, we outline its architecture and provide concrete examples of how OpenSim Creator has already been used to build new biomechanical models.


# Statement of need

OpenSim [@Delp:2007; @Seth:2018] is an open-source musculoskeletal modeling platform with a large and growing number of users in diverse fields. This follows the general trend of musculoskeletal modeling being increasingly used in clinical [@Marconi:2023], biomechanical [@Porsa:2016], and palaeobiological [@Beesel:2021; @Demuth:2023] studies. However, creating a biomechanical model remains a challenge because the process is both laborious, as it requires the input of a large amount of biological data, and complex, as it requires expertise in biomedical imaging, model/data file formats, and musculoskeletal anatomy.

As an illustrative example, @Bishop:2021 outline a workflow for building a biomechanical model of a dinosaur. It involves specialized steps such as a) acquiring and segmenting bone morphology, centers of mass, and inertia tensors from fossil records; b) rigging the bones into a skeletal model; c) reconstructing musculature from soft tissue volumes; and d) using the resulting model in a variety of biomechanical simulations. Specialized procedures like these may need to be repeated for each subject in an experiment because animals are unique, which can significantly increase the amount of labor required. Scaling existing models can minimize that work, but designing a correct, unbiased [@vanDerKruk:2025], scaling procedure that is capable of handling a variety of shapes, injuries, and deformities is also challenging.

Similar challenges have been addressed in fields such as 3D design, engineering, and game development. For example, engineers commonly use computer-aided design (CAD) software both to reduce labor, by automating actions such as 3D object placement; and to increase the user's expertise, by enabling model editing in a safe undoable area with immediate, useful, visual feedback. As musculoskeletal modelling with OpenSim expands to fields that are not traditionally occupied by engineers (e.g., medicine, palaeobiology), real-time model development with visual feedback becomes paramount. OpenSim Creator provides that tooling by building on ideas from existing CAD software - with a biomechanical twist.


# Design and implementation

We initially studied existing open-source tools (e.g., @Blender) to find common design and architectural themes. We found that, in most cases, they focus on providing immediate visual feedback whenever a user performs an action and are continuously improved over many releases. Immediate feedback is usually achieved by directly integrating the UI with the underlying state/model representation (as opposed to relying on an indirect mechanism, such as a remote procedure call). Based on that, and our previous experience with OpenSim, we decided that OpenSim Creator's architecture should focus on the following four key attributes:

- **High performance**: to enable real-time model editing and instant feedback.
- **Low-level UI control**: to enable a highly customized user experience. Specialized tasks in biomechanical model building (e.g. landmark placement, mesh reorientation, mesh warping, real-time data visualization) can require low-level control over the rendering process.
- **Direct integration with the OpenSim API**: to directly access and visualize biomechanical model quantities (e.g. muscle lengths, moment arms) on-demand with minimal development or performance overhead.
- **Iterative software, and model, development**: to enable rapid software prototyping and feedback from the research community.

To facilitate those requirements, OpenSim Creator took inspiration from the games industry, which is commonly associated with rapid prototyping and real-time rendering. We developed OpenSim Creator using a combination of C++, a high-performance language that directly integrates with the OpenSim API; a custom OpenGL renderer, to tightly control real-time 3D editing; and Dear ImGui (@ImGui), which provides customizable low-level 2D UI.

Organizationally, we adopted an open, iterative, development methodology. All releases of OpenSim Creator have been available on GitHub with an Apache 2.0 license as early as the first month of development, with new releases published every 1-2 months. As a consequence, we have been able to continuously receive---and implement---feedback from researchers in the community over 36 release cycles. This is in addition to a healthy supply of real model-building challenges from our own research, which contributed heavily to OpenSim Creator's design and architecture (e.g. \autoref{fig:1}).

![Screenshots of OpenSim Creator's specialized workflows. a) The mesh importer workflow, which focuses on reorienting and rigging meshes in a free-form editor, b) the mesh warper workflow, which focuses on placing corresponding landmarks on meshes (left and middle) with real-time updates to a Thin-Plate Spline [@Bookstein:1989] warped mesh (right). \label{fig:1}](images/fig1.png)


# Example use-cases

Subject-specific models are necessary because all individuals are unique and can have a wide variety of injuries and deformities. Our goal was to create a subject-specific model of a human shoulder. A previously published [@Seth:2019] generic shoulder model was used as a template by superimposing it over subject-specific CT scans in OpenSim Creator's mesh importer UI (\autoref{fig:1}a). The mesh importer UI enabled real-time, free-form placement and orientation of the subject-specific joints and bodies. The scene was automatically converted into an OpenSim model. Muscle volumes from subject-specific MRI data were combined with moment arm data from the generic model to create a line of action for each muscle in the model. The model editor UI provided real-time visual feedback about how each muscle's moment arm was affected by the placement of attachment points and wrapping surfaces on the line of action within the model (\autoref{fig:2}).

Another study evaluated how a given biomechanical model simulated a sit-to-stand movement. The predictive simulations proved to be computationally expensive, and OpenSim Creator was used to simplify the existing model by eliminating wrapping surfaces and leveraging quick visual feedback (both as 3D renders and 2D plots) to place muscle via points while preserving the moment-arms of the original model (@Heijerman:2023).

![A screenshot of OpenSim Creator's OpenSim model editor UI, showing a subject-specific human shoulder model made using it. The model editor can host multiple panels, each of which are updated whenever the user edits the model. For example, the muscle plotter (right) can be used to show how a muscle's moment arm changes as a user edits the muscle in the 3D viewport (middle) or in the properties panel (left).\label{fig:2}](images/fig2.png)


# Conclusions

By combining a rapid-prototyping architecture inspired by the games industry with a development approach that continuously incorporates feedback from the biomechanical research community, we designed and developed a graphical UI that enables the creation and evaluation of OpenSim models more quickly and easily.

OpenSim Creator's design and architecture enables it to render multiple high-vertex-count bone models in real-time on a mid-range laptop. Its control over the UI enables customized workflows where researchers can create models visually, and its tight integration with OpenSim enables it to show moment arms in real time as model creation proceeds. Our users report significantly reduced amounts of time and effort to build, edit, and evaluate models when compared to existing approaches.

As software constantly evolves, we are looking forward and anticipate leveraging OpenSim Creator to create specialized workflows for visually-assisted model warping, real-time sensor visualization, and the import of other biomedical data, in addition to continuing to improve the day-to-day user experience of creating an OpenSim model from medical imaging and emerging sources of anatomical structure and material properties.


# Acknowledgements

This project was sponsored by grant numbers 2020-218896 and 2022-252796 from the Chan Zuckerberg Initiative DAF, an advised fund of Silicon Valley Community Foundation. It was also sponsored by the Dutch Research Council, NWO XS: OCENW.XS21.4.161. All sponsors' involvement was exclusively financial.

We thank DirkJan Veeger and Bart Kaptein for shoulder CT and MRI data for the human model.


# References
