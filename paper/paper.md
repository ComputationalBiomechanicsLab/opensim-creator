---
title: 'OpenSim Creator: A Graphical User Interface for Building OpenSim Models'
tags:
  - C++
  - UI
  - OpenSim
  - Musculoskeletal Modelling
  - Biomechanics
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
date: 30 April 2024
bibliography: paper.bib
---


# Summary

Here, we present OpenSim Creator, an open-source “biomechanical CAD” user interface (UI) that focuses on building and evaluating OpenSim models. We also present examples of how OpenSim Creator has already been used to build new biomechanical models.


# Introduction

OpenSim [1,2] is an open-source musculoskeletal modelling platform that has received increased interest for building new biomechanical models over the past few years [2-5]. This follows the general trend of musculoskeletal modelling being increasingly used in clinical [3], biomechanical [4], and palaeobiological [5] studies. However, creating a biomechanical model remains a challenge because the process is both laborious, as it requires the input of a large amount of biological data, and complex, as it requires expertise in biomedical imaging, model and data file formats, and musculoskeletal anatomy.

The essence of that challenge—to streamline a building process—is also present in fields such as 3D design, engineering, and game development. For example, engineers commonly use computer-aided design (CAD) software both to reduce labour, by automating actions (e.g., model validation); and to increase expertise, by enabling model editing in a safe undoable area with immediate, useful, visual feedback. OpenSim Creator's aim is to continue the heritage CAD software, but with a biomechanical twist on the theme.


# Design/Implementation Method

We initially studied existing open-source CAD software (e.g., Blender [6]) to find common design themes. We found that they typically rely on real-time editing capabilities to provide immediate visual feedback in response to each user action. We also found that they are constantly evolving: all successful open-source CAD applications we studied are continuously updated and improved. Based on that, we anticipated that OpenSim Creator’s architecture would require the following four key attributes:

- **High performance**: to accommodate real-time model editing.
- **Low-level UI control**: to accommodate highly customized visual feedback.
- **Tight integration with the OpenSim API**: to accommodate showing biomechanical information (e.g. muscle lengths, moment arms) on-demand, and to run forward dynamic simulations directly inside the UI.
- **Iterative software, and model, development**: to accommodate rapid software prototyping and feedback from the research community.

To facilitate those requirements, OpenSim Creator took inspiration from the games industry, which is commonly associated with rapid prototyping and real-time rendering. We developed OpenSim Creator using a combination of C++, a high-performance language that directly integrates with the OpenSim API; a custom OpenGL renderer, to tightly control real-time 3D editing; and ImGui [7], which provides customizable low-level 2D UI.

We adopted an open, iterative, development methodology. All releases, including alpha releases, of OpenSim Creator have been available on GitHub with an Apache license as early as the first month of development, and a new releases are published every 1-2 months. As a consequence, we have been able to continuously receive feedback from researchers in the community, in addition to a healthy supply of real model-building challenges from our own research, which contributed heavily to OpenSim Creator's design and architecture (e.g. \autoref{fig:1}).

![A selection of screenshots of OpenSim Creator, demonstrating its flexible UI architecture for real-time 3D editing. a) mesh importer, c) mesh warper, c) via point optimization (from [9]). \label{fig:1}](images/fig1.png)


# Results and Discussion

With the aim of demonstrating how the design and architectural methods outlined above interplay with the model-building process, we will discuss concrete examples of how models have been built using OpenSim Creator.

Subject-specific models are necessary when (e.g.) a subject has deformities that cannot be adequately represented by scaling a generic model. Our goal was to create a subject-specific model of a human shoulder. A previously published [8] generic shoulder model was used as a template by superimposing it over subject-specific CT scans in OpenSim Creator’s mesh importer UI (\autoref{fig:1}a). The mesh importer UI enabled real-time, freeform placement and orientation of the subject-specific joints and bodies. The scene was automatically converted into an OpenSim model. Muscle volumes from subject-specific MRI data were combined with moment arm data from the generic model to create a line of action for each muscle in the model. The model editor UI provided real-time visual feedback about how each muscle’s moment arm was affected by the placement of attachment points and wrapping surfaces on the line of action within the model (\autoref{fig:2}).

Another study evaluated how a given biomechanical model simulated a sit-to-stand movement. The predictive simulations proved to be computationally expensive, and OpenSim Creator was used to simplify the existing model by eliminating wrapping surfaces and leveraging quick visual feedback to place muscle via points while preserving the moment-arms of the original model (\autoref{fig:1}c, [9]).

![OpenSim Creator’s model editor UI, showing a subject-specific human shoulder model made using it. The model editor can host multiple panels, each of which are updated whenever the user edits the model. For example, the muscle plotter (right) can be used to show how a muscle’s moment arm changes as a user edits it in the 3D viewport (middle) or in the properties panel (left).\label{fig:2}](images/fig2.png)


# Conclusions

By combining a rapid-prototyping architecture inspired by the games industry with a development approach that continuously incorporates feedback from the biomechanical research community, we designed and developed a UI that enables researchers to create and evaluate OpenSim models more quickly and easily.

OpenSim Creator’s design and architecture enabled it to render multiple high-vertex-count CT scans in a single scene in real-time on a mid-range laptop. Its control over the UI enables customized workflows where researchers can create models visually, and its tight integration with OpenSim enables it to show moment arms in real time as model creation proceeds.  Our users report significantly reduced amounts of effort required when compared to existing approaches.

Software constantly evolves. Looking forward, we anticipate that OpenSim Creator’s architecture could be used to create specialized workflows for (e.g.) visually-assisted model warping, real-time sensor visualization, and the import of other biomedical data, in addition to improving the day-to-day user experience of creating an OpenSim model.


# Acknowledgements

This project has been made possible in part by grant numbers 2020-218896 and 2022-252796 from the Chan Zuckerberg Initiative DAF, an advised fund of Silicon Valley Community Foundation and the Dutch Research Council, NWO XS: OCENW.XS21.4.161

We thank DirkJan Veeger and Bart Kaptein for shoulder CT and MRI data for the human model.


# References (TODO)

1. Delp SL, et. al. IEEE Transactions on Biomedical Engineering, 54(11): 1940-1950, 2007.
2. Seth A, et al. PLoS Comput Biol 14(7): e1006223, 2018.
3. Marconi G, et al. Med Biol Eng Comput , 2023, doi:10.1007/s11517-023-02778-2. 
4. Porsa S, et al. Ann Biomed Eng 44: 2542–2557, 2016.
5. Demuth OE, et al. R Soc open sci 10(1): 221195, 2023.
6. https://www.blender.org/
7. https://github.com/ocornut/imgui
8. Seth A et al. Front Neurorobot 13: 90, 2019.
9. Heijerman S, Master thesis, TUDelft repository, 2023.

- A list of key references including a link to the software archive.
- citations must refer to bibtex key (e.g. "Something worth citing [@Pearson:2017]")
- For a quick reference, the following citation commands can be used:
  - `@author:2001`  ->  "Author et al. (2001)"
  - `[@author:2001]` -> "(Author et al., 2001)"
  - `[@author1:2001; @author2:2001]` -> "(Author1 et al., 2001; Author2 et al., 2002)"
