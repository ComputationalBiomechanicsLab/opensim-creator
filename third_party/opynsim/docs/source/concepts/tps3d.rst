.. currentmodule:: opynsim.tps3d

Thin-Plate Spline (TPS) Warping
===============================

The :mod:`~opynsim.tps3d` module contains classes/functions related to using
the Thin-Plate Spline (TPS) technique with 3D data, this page provides a
technical overview of the technique and how to use it in OPynSim.


Overview
--------

TPS is a non-linear scaling (warping) technique in which points (landmarks) from
a "source" coordinate system are paired with points in a "destination" coordinate
to compute a warping interpolant. This establishes a non-linear mapping between
two coordinate systems, which can be useful for tasks like subject-specific
scaling.

The primary literature source for the TPS technique is:

    F.L. Bookstein, "Principal warps: thin-plate splines and the decomposition of deformations," in IEEE
    Transactions on Pattern Analysis and Machine Intelligence, vol. 11, no. 6, pp. 567-585, June
    1989, doi: `10.1109/34.24792 <https://doi.org/10.1109/34.24792>`_.

    See also: `Relevant Literature`_

A high-level mathematical overview of the TPS technique in three dimensions is that warping a
point in :math:`\mathbb{R}^3` (:math:`v`) involves evaluating this equation (referred to as
"warping equation", or "interpolant"):

.. math::

    f(v) = a_1 + a_2v_x + a_3v_y + a_4v_z + \sum_{i=1}^K w_i ||u_i - v||

Where :math:`a_1`, :math:`a_2`, :math:`a_3`, :math:`a_4`, :math:`w_i..w_K`, and :math:`u_i..u_K`
are :math:`\mathbb{R}^3` vector coefficients computed from :math:`K` source + destination landmark
pairs. In OPynSim, solving the coefficients is equivalent to calling :meth:`solve_coefficients`,
evaluating :math:`f(v)` is equivalent to calling :meth:`TPSCoefficients3D.warp_point`.


API Usage
---------

The :mod:`~opynsim.tps3d` module provides APIs for the TPS technique. The workflow is as follows:

- Call :func:`solve_coefficients` with landmark pairs, which returns...
- :class:`TPSCoefficients3D`, a representation of the warping equation's coefficients (i.e. :math:`a_1`,
  :math:`a_2`, etc.), which has methods like...
- :meth:`TPSCoefficients3D.warp_point`: a method that evaluates the warping equation for a single
  point.

A concrete integration of the API (e.g. as in `OpenSim Creator <https://www.opensimcreator.com>`_)
looks something like this:

- Landmarks are loaded, paired, and validated from data files (e.g. CSV files, user
  selection).
- The paired landmarks are used to solve the TPS coefficients with :meth:`solve_coefficients`.
  Higher-level systems may use techniques such as hashing and equality checks to cache
  this step.
- The resulting :class:`TPSCoefficients3D` can then used to warp other data, such as mesh
  vertices, and muscle insertion points.


Implementation Notes
-------------------

Basis Function
~~~~~~~~~~~~~~

OPynSim's TPS implementation uses :math:`U(u) = ||u||` as its basis function. It is mentioned
in the following literature source:

    Gunz, P., Mitteroecker, P., Bookstein, F.L. (2005). Semilandmarks in Three
    Dimensions. In: Slice, D.E. (eds) Modern Morphometrics in Physical Anthropology. Developments in
    Primatology: Progress and Prospects. Springer, Boston, MA. https://doi.org/10.1007/0-387-27614-9_3

This is in contrast to the primary Bookstein source, which uses :math:`U(u) = ||u||^2 log ||u||^2`. The
choice of basis function is mostly an implementation detail, noted here for completeness. However, it
may be important when considering results from other TPS implementations, or when using ``warping_penalty``
(see below).


Warping Penalty
~~~~~~~~~~~~~~~

The OPynSim :meth:`solve_coefficients` API also includes a ``warping_penalty`` term, which is not
specified in the primary literature source. Colloquially, this parameter is a regularizer that
penalizes smoothing/warping. This makes the warping equation (interpolant) more affine in
behavior. A ``warping_penalty`` of ``0`` (default) has no effect on the coefficient solver.

Why do this? Because, with no constraint on smoothness, the interpolant can produce implausible
local deformations. These manifest as visible artefacts and unpredictable warping - particularly
in regions where landmarks are sparse or unevenly distributed relative to the geometry being warped.

As a concrete example, in the personalized musculoskeletal modelling workflow described in the
following literature source:

    Stansfield et al. (2026, PLOS Computational Biology, `10.1371/journal.pcbi.1014073 <https://doi.org/10.1371/journal.pcbi.1014073>`_)

They encountered those issues until they starting using something similar to OPynSim's ``warping_penalty``. In
that study, they were using the `thin-plate-spline  <https://github.com/raphaelreme/tps>`_ Python package,
which exposes the same regularizer (named ``alpha``). The researchers tuned the regularizer experimentally
so that the warped points stayed within anatomically plausible bounds while reconstructed bone surfaces
remained visually smooth.

``warping_penalty`` Implementation Details
""""""""""""""""""""""""""""""""""""""""""

In the primary literature source, the following linear system is solved:

.. math::

   \begin{bmatrix}
   K   & P \\
   P^T & \mathbf{0}
   \end{bmatrix}
   \begin{bmatrix}
   \mathbf{w} \\
   \mathbf{a}
   \end{bmatrix}
   =
   \begin{bmatrix}
   \mathbf{v} \\
   \mathbf{0}
   \end{bmatrix}

Where (simplifying) :math:`K` represents a symmetric matrix containing each landmark pair combination
evaluated via the basis function (:math:`U`), :math:`P` represents a matrix containing the source
landmark positions, :math:`\mathbf{a}` / :math:`\mathbf{w}` represent vectors containing the
affine/non-affine terms of the warping equation (interpolant), and :math:`\mathbf{v}` represents
a vector of the destination landmark positions.

With ``warping_penalty`` (:math:`\alpha`), this becomes:

.. math::

   \begin{bmatrix}
   K + \alpha I   & P \\
   P^T & \mathbf{0}
   \end{bmatrix}
   \begin{bmatrix}
   \mathbf{w} \\
   \mathbf{a}
   \end{bmatrix}
   =
   \begin{bmatrix}
   \mathbf{v} \\
   \mathbf{0}
   \end{bmatrix}

Which has the effect of penalizing (if :math:`\alpha > 0`) non-affine coefficients (:math:`w`) when
the linear system is solved. Because ``warping_penalty`` is just a constant added along
:math:`K`'s diagonal, the magnitude of its regularization effect is dependent on the magnitude
of the diagonal, which is dictated by the input data (landmark pairs) and the basis
function (:math:`U`).

Practically speaking, this means that ``warping_penalty`` is a number that must be tweaked on
a per-use-case basis, rather than it being (e.g.) a normalized factor with well-defined behavior
and standardized endpoints. As a practical guideline, start with ``warping_penalty=0.0`` (the
default) and experiment with gradually increasing it when artifacts appear.


TPS Nomenclature and Etymology
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some literature sources use the word "bending" to describe the non-affine component of
the transformation. This etymology is linked to the underlying concept of minimizing the
energy required to bend a thin infinitely-elastic metal plate, as described in the
original Bookstein literature source. Generally, "bending" is more commonly used in
statistics, morphometrics, and mathematics.

Other literature sources use the word "warping". This etymology is rooted in computer
graphics, image processing, and geometric modelling, where the word is used to describe
non-linear distortions of 2D images or 3D meshes. OPynSim uses "warping" because the
technique was originally developed as part of a general **model warping** pipeline
for `OpenSim Creator <https://www.opensimcreator.com>`_, where TPS isn't necessarily
exclusively used.



.. _Relevant Literature:

Relevant Literature
-------------------

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
- Do we need medical imaging-informed musculoskeletal models for simulations in healthy adults? A new workflow based on magnetic resonance imaging highlights the importance of personalized geometry (`link <Do we need medical imaging-informed MSK models>`_)
    - Uses something similar to ``warping_penalty`` (:meth:`solve_coefficients`) in an academic context.
- ``thin-plate-spline``: Open-Source Python TPS implementation (`link <thin-plate-spline Python Package>`_)
    - Includes regularization support (called ``warping_penalty`` in OPynSim, ``alpha`` in ``thin-plate-spline``).

.. _TPS General Info: https://en.wikipedia.org/wiki/Thin_plate_spline
.. _TPS Primary Literature Source: https://ieeexplore.ieee.org/document/24792
.. _TPS Basic Explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
.. _TPS Warping Blog Post: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
.. _Image Warping and Morphing: http://groups.csail.mit.edu/graphics/classes/CompPhoto06/html/lecturenotes/14_WarpMorph.pdf
.. _Thin-Plate Spline C++ Demo: https://elonen.iki.fi/code/tpsdemo/
.. _CThinPlateSpline: https://github.com/tractatus/fisseq/blob/master/src/CThinPlateSpline.h
.. _Interactive Thin-Plate Spline Interpolation: https://github.com/sarathknv/tps
.. _3D Thin Plate Spline Warping Function: https://uk.mathworks.com/matlabcentral/fileexchange/37576-3d-thin-plate-spline-warping-function
.. _3D Point set warping by thin-plate/rbf function: https://uk.mathworks.com/matlabcentral/fileexchange/53867-3d-point-set-warping-by-thin-plate-rbf-function
.. _Do we need medical imaging-informed MSK models:  https://doi.org/10.1371/journal.pcbi.1014073
.. _thin-plate-spline Python Package: https://github.com/raphaelreme/tps
