Thin-Plate Spline (TPS) Warping
===============================

The :mod:`opynsim.tps3d` module contains classes/functions related to using
the Thin-Plate Spline (TPS) technique with 3D data.

Overview
--------

TPS is a non-linear scaling technique that requires corresponding pairs of
points (landmarks) from both a "source" and "destination" to create warped "result"
points. OPynSim's implementation of TPS was ported from
`OpenSim Creator <https://www.opensimcreator.com/>`_, which provides a concrete use-case
for TPS in its `mesh warper documentation <https://docs.opensimcreator.com/manual/en/latest/the-mesh-warper.html>`_. This
documentation page is a brief explanation of the primary literature sources on the
topic of TPS.

The primary literature source for the TPS technique is:

    F.L. Bookstein, "Principal warps: thin-plate splines and the decomposition of deformations," in IEEE
    Transactions on Pattern Analysis and Machine Intelligence, vol. 11, no. 6, pp. 567-585, June
    1989, doi: `10.1109/34.24792 <https://doi.org/10.1109/34.24792>`_.

    See also: `Relevant Literature`_

A high-level view of the TPS technique in three dimensions is that warping a 3D
point (:math:`v`) involves evaluating this equation (referred to as "warping
equation" in this documentation page):

.. math::

    f(v) = a_1 + a_2v_x + a_3v_y + a_4v_z + \sum_{i=1}^K w_i ||u_i - v||

Where :math:`a_1`, :math:`a_2`, :math:`a_3`, :math:`a_4`, :math:`w_i..w_K`, and :math:`u_i..u_K`
are :math:`\mathbb{R}^3` vector coefficients computed from :math:`K` source + destination landmark
pairs. Evaluating :math:`f(v)` is equivalent to calling :meth:`opynsim.tps3d.TPSCoefficients3D.warp_point`.

.. note::

    OPynSim's warping equation assumes a basis function (:math:`U`) of :math:`U(u) = ||u||`, from:

        Gunz, P., Mitteroecker, P., Bookstein, F.L. (2005). Semilandmarks in Three
        Dimensions. In: Slice, D.E. (eds) Modern Morphometrics in Physical Anthropology. Developments in
        Primatology: Progress and Prospects. Springer, Boston, MA. https://doi.org/10.1007/0-387-27614-9_3

    Rather than :math:`U(u) = ||u||^2 log ||u||^2`, from the Bookstein source. This is because it tends to
    behave better. The choice of basis function is an implementation detail, noted here for completeness.

OPynSim provides a Python API for the TPS technique, in the form of:

- :func:`opynsim.tps3d.solve_coefficients`: a function that computes the warping equation
  coefficients (:math:`a_1`, :math:`a_2`, etc.) for the provided landmark pairs, which returns...
- :class:`opynsim.tps3d.TPSCoefficients3D`: a class that represents the coefficients, which has...
- :meth:`opynsim.tps3d.TPSCoefficients3D.warp_point`: a method that evaluates the warping
  equation (:math:`f`, above) for a single point.

As a high-level workflow example, a script that uses OPynSim script could load/gather
landmark pairs (e.g. from scanner data, CSV files), provide them to :func:`opynsim.tps3d.solve_coefficients`,
and then use methods like :meth:`opynsim.tps3d.TPSCoefficients3D.warp_point` to warp other things
from the "source" coordinate system to the warped/result coordinate system (e.g. muscle via points).


API Reference
-------------

.. autofunction:: opynsim.tps3d.solve_coefficients

.. automodule:: opynsim.tps3d
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:


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
