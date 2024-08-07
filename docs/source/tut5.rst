.. _tut5:


Use the Mesh Warper
===================

In this tutorial, we will be using the Mesh Warping UI to perform
landmark-driven mesh warping using the Thin-Plate Spline algorithm
(`explanation <TPS General Info_>`_, `literature source <TPS Primary Literature Source_>`_). The UI
provides tooling for loading two meshes and creating landmark pairs between them:

.. _cylinder-warp-example:
.. figure:: _static/tut5_mesh-warper-screenshot.png
    :width: 60%

    The mesh warping UI, which shows the source ("reference", left) destination ("target", middle) and result ("warped", right) meshes.
    Here, the warping quality is low. This is because of the low triangle and landmark count.

.. figure:: _static/tut5_mesh-warper-organic-screenshot.png
    :width: 60%

    Same as :numref:`cylinder-warp-example`, but showing an example of warping a clavicle bone. This example has many paired (left-to-middle) landmarks in a
    variety of locations along the surface of the bone, which improves the warp quality (right).

Prerequisites
-------------

* **This is a standalone tutorial**. The mesh warping UI is designed to be separate
  from OpenSim, to specifically address the desire to pair landmarks and visualize
  non-uniform warping.
* **For your own work**, you will need two meshes that are anatomically pair-able
  (e.g. two femur meshes that you'd like to pair+warp with landmarks)


Topics Covered by this Tutorial
-------------------------------

- An overview of the Thin-Plate Spline technique
- How OpenSim Creator's mesh warping UI works with that technique
- A concrete walkthrough of using the UI on an anatomical mesh


The Thin-Plate Spline (TPS) Technique
-------------------------------------

.. note::

    This section isn't going to explain the Thin-Plate Spline (TPS) technique in
    extensive detail. Instead, it will provide a simplified explanation
    that's good enough to get a rough idea of what's happening when you use the
    mesh warping UI.

    If you want something more, we recommend consulting the `Relevant References`_,
    which lists a variety of content related to the TPS technique.


Imagine placing a thin plate with points along its surface onto a table. Now
imagine that each surface point has a corresponding "target" point somewhere
in 3D space. You could minimize the distance between those corresponding points
by physically bending the plate, but you'll need to figure out the optimal way
to bend it.

The TPS technique models this principle by:

- Describing "bending" the plate as a bounded linear combination of some
  basis function, :math:`U(v)`. The `original paper <TPS Primary Literature Source_>`_
  used :math:`U(v) = |v|^2 \log{|v|^2}`, but `other sources <SemilandmarksInThreeDimensions_>`_ use :math:`U(v) = |v|`.
- Treating the problem of transforming "source/reference" points (landmarks),
  :math:`x_i`, to "destination/reference" points (landmarks), :math:`y_i`, as an
  interpolation problem.
- Solving the coefficients of that linear combination while minimizing the
  "bending energy". `Wikipedia example <TPS General Info_>`_:

.. math::

    E_{\mathrm{tps}}(U) = \sum_{i=1}^K \|y_i - U(x_i) \|^2

The coefficients that drop out of this process can then be used to warp any
point in the same space. **However**, remember that this explanation is 
simplifying things, and that you should read `Relevant References`_ if
you want to learn more.

Here's how concepts from TPS apply to OpenSim Creator's mesh warping UI:

- OpenSim Creator's mesh warper uses the TPS technique. It's conceptually
  handy to think about how that can be used to "bend" mesh surfaces.

- **Source Mesh** and **Source Landmark** refer to data in the "reference", or
  "source" space. Each source landmark requires a corresponding destination
  landmark with the same name.

- **Destination Mesh** and **Destination Landmark** refer to data in the "target", or
  "destination" space. Each destination landmark must have a corresponding
  source landmark with the same name.

- **Warp Transform** is the product of the TPS technique after pairing the
  landmarks and solving the relevant coefficients. The transform can be applied
  to point data to warp it. E.g. in the mesh warping UI, the transform is applied
  to the source mesh to produce the result mesh. It's also applied to "non-participating landmarks"
  to produce warped points.

- **Result Mesh** is the result of applying the warp transform to the source mesh.

- **Non-Participating Landmark** is a landmark in the source mesh's space that
  should be warped by the warp transform but shouldn't participate in solving
  the TPS coefficients.


Opening the Mesh Warping UI
---------------------------

.. figure:: _static/tut5_open-mesh-warper-from-splash-screen.png
    :width: 60%

    The mesh warping UI can be opened from the main splash screen of OpenSim Creator
    (highlighted red).


Mesh Warping UI Overview
------------------------

.. figure:: _static/tut5_mesh-warper-organic-screenshot.png
    :width: 60%

    Screenshot of the mesh warping UI, with the (optional) "Landmark Navigator" window
    open on the right-hand side.

Although the TPS technique (explained above) only actually requires paired
landmarks in 3D space, the mesh warping UI pairs those landmarks with the surfaces
of meshes. This pairing is natural when working with physiological meshes, because
"a pair of 3D points" isn't as useful as "the most posterior point on a femur's
lateral condyl in two CT scans" when we want to use those correspondences to
warp associated data (e.g. muscle points).

The mesh warping UI separates the information into three panels:

- **Source**: "source", or "reference" meshes and landmarks. These are where
  you're starting from. In this panel, you can ``Import`` meshes, landmarks,
  and non-participating landmarks (datapoints, such as markers, that should
  be warped, but shouldn't participate in fitting TPS parameters).


* TODO: Explain top-level UI design (what each panel does)
* TODO: Explain key features (import, export, blending, etc.)


Example: Warping a Femur
------------------------

* TODO: Provide + Load Example Meshes
* TODO: Step-by-step explanation
* TODO: warping single points/non-participating landmarks


.. _Relevant References:

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
.. _SemilandmarksInThreeDimensions: https://doi.org/10.1007/0-387-27614-9_3
