.. _tut5:


Use the Mesh Warper
===================

In this tutorial, we will be using the Mesh Warping UI to perform
landmark-driven mesh warping using the Thin-Plate Spline algorithm
(`explanation <TPS General Info_>`_, `literature source <TPS Primary Literature Source_>`_). The UI
provides tooling for loading two meshes and creating landmark pairs between them.


Prerequisites
-------------

* **This is a standalone tutorial**. The mesh warping UI is designed to be separate
  from OpenSim, to specifically address the desire to pair landmarks and visualize
  non-uniform warping.
* **For your own work**, you will need two meshes that are anatomically pair-able
  (e.g. two femur meshes that you'd like to pair+warp with landmarks)


Topics Covered by this Tutorial
-------------------------------

- A general overview of the Thin-Plate Spline technique
- A general overview of the mesh warping UI
- Using the mesh warping UI on a basic anatomical example


The Thin-Plate Spline Technique
-------------------------------

TODO: Basic overview of the high-level idea (minimal warping of a thin plate of metal), how
it relates to the maths (fitting points in n-dimensions), how it relates to landmarks (3D corresponding points) and
where meshes (they're clouds of points that the resulting warp polynomial can be applied to) and "non-participating landmarks" (e.g. one-off datapoints)
fit into the picture.


The Mesh Warping UI
-------------------

TODO

Opening the UI
~~~~~~~~~~~~~~

TODO

Panels in the UI
~~~~~~~~~~~~~~~~

TODO

Points of Interest in the UI (import, export, blending, etc.)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TODO


Example: Warping a Femur
------------------------

TODO

Load Example Meshes
~~~~~~~~~~~~~~~~~~~

TODO

Step-By-Step Explanation
~~~~~~~~~~~~~~~~~~~~~~~~

TODO

Warping Single Points (markers, muscle points)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TODO


Relevant References
-------------------

These references were found during the development of OpenSim Creator's mesh
warping support (`issue #467 <OSC TPS Github Issue_>`_). They are here in case
you (e.g.) want to write about this subject, or create your own implementation of
the algorithm.

- Wikipedia: Thin-Plate Spline (`link <TPS General Info_>`_)
    - Top-level explanation of the algorithm
- Principal warps: thin-plate splines and the decomposition of deformations, Bookstein, F.L. (`link <TPS Primary Literature Source_>`_)
    - Primary literature source
    - Note: newer publications tend to use a different basis function
- Manual Registration with Thin Plates, Herve Lombaert (`link <TPS Basic Explanation_>`_)
    - Easy-to-read explanation of the underlying maths behind the Thin-Plate Spline algorithm
    - Useful as a basic overview
- Thin Plates Splines Warping, Khanh Ha (`link <TPS Warping Blog Post_>`_)
    - Explanation of the low-level maths behind the Thin-Plate Spline algorithm (e.g. radial basis functions). Includes concrete C/C++/OpenCV examples
    - Useful as a basic overview for C++ implementors
- Image Warping and Morphing, Frédo Durand (`link <Image Warping and Morphing_>`_)
    - Full presentation slides that explain the problem domain and how warping can be used to solve practical problems, etc. Explains some of the low-level maths very well (e.g. RBFs) and is a good tour of the field. Does not contain practical code examples.
    - Useful as a top-level overview of warping in general
- Thin Plate Spline editor - an example program in C++, Jarno Elonen (`link <Thin-Plate Spline C++ Demo_>`_)
    - C++/OpenGL/libBLAS implementation of the TPS algorithm
    - Useful for implementors
- CThinPlateSpline.h, Daniel Fürth (`link <CThinPlateSpline_>`_)
    - C++/OpenCV Implementation
    - Useful for implementors
- Interactive Thin-Plate Spline Interpolation, Sarath Chandra Kothapalli  (`link <Interactive Thin-Plate Spline Interpolation_>`_)
    - Basic python implementation of TPS using numpy and matlab.
    - Contains basic explanation of the algorithm in the README
    - Useful for implementors
- 3D Thin Plate Spline Warping Function, Yang Yang (`link <3D Thin Plate Spline Warping Function_>`_)
    - MATLAB implementation of the algorithm
    - Useful for implementors
- 3D Point set warping by thin-plate/rbf function, Wang Lin (`link <3D Point set warping by thin-plate/rbf function_>`_)
    - MATLAB implementation of the algorithm
    - Useful for implementors
- A Practical Guide to Sliding and Surface Semilandmarks in Morphometric Analyses, Bardua, C et. al. (`link <A Practical Guide to Sliding and Surface Semilandmarks in Morphometric Analyses_>`_)
    - Introduces a UX for placing semi-landmarks (not supported by OpenSim Creator yet)
    - Useful for UI implementors

.. _TPS General Info: https://en.wikipedia.org/wiki/Thin_plate_spline
.. _TPS Primary Literature Source: https://ieeexplore.ieee.org/document/24792
.. _OSC TPS Github Issue: https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/467
.. _TPS Basic Explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
.. _TPS Warping Blog Post: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
.. _Image Warping and Morphing: http://groups.csail.mit.edu/graphics/classes/CompPhoto06/html/lecturenotes/14_WarpMorph.pdf
.. _Thin-Plate Spline C++ Demo: https://elonen.iki.fi/code/tpsdemo/
.. _CThinPlateSpline: https://github.com/tractatus/fisseq/blob/master/src/CThinPlateSpline.h
.. _Interactive Thin-Plate Spline Interpolation: https://github.com/sarathknv/tps
.. _3D Thin Plate Spline Warping Function: https://uk.mathworks.com/matlabcentral/fileexchange/37576-3d-thin-plate-spline-warping-function
.. _3D Point set warping by thin-plate/rbf function: https://uk.mathworks.com/matlabcentral/fileexchange/53867-3d-point-set-warping-by-thin-plate-rbf-function
.. _A Practical Guide to Sliding and Surface Semilandmarks in Morphometric Analyses: https://doi.org/10.1093/iob/obz016
