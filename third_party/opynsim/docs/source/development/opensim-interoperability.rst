OpenSim Interoperability
========================

This page serves as central source that developers can read to find out
how OPynSim handles its interoperability with OpenSim.

OPynSim's implementation is designed, built, packaged, and deployed
entirely separately from OpenSim. This enables it to have a different
development process/cadence and API design from OpenSim. However, although
the implementation and API may differ in the details, a major goal of
OPynSim is to always provide strong interoperability with OpenSim's
**data formats**, so that it's capable of both reading and writing
data that's compatible with tools in the wider OpenSim ecosystem.

.. note::

    OPynSim doesn't aim to emulate or reproduce the original OpenSim
    API. The OpenSim project already ships C++ and Python APIs. You
    should use it, where applicable. OPynSim ships a different API
    that is specifically catered for Python interoperability.


File Format Interoperability
----------------------------

It's important that OPynSim supports reading and writing the most
commonly used data formats used in the OpenSim ecosystem, not
just ``.osim`` model files. It acts as one infrastructural
library that handles the users' most common data processing
requirements.

To get an idea of the distribution of data files in the OpenSim
ecosystem, 226 of the most popular OpenSim studies were collected
from SimTK.org. Their contents were scanned for file extensions to
find the most common file formats used by researchers. The table
below contains the top 21 (97 %) most-used formats in the collection
(N=84847), with comments.

OPynSim currently provides parsers for around 76 % of the
collection. Python itself provides parsers for 5 % (``.xml``, ``.pkl``). Proprietary
MATLAB-specific files account for 8 %. The rest are either human-readable
plaintext (4.6 %; ``.log``, ``.txt``, ``.asc``), or easily converted
with third-party software (1.9 %, ``.ply``). Which is to say, OPynSim
provides excellent coverage, with a uniform API, for data that cannot
easily be handled elsewhere.

========== ============== =============================================================================
Format     Count          Comments
========== ============== =============================================================================
.sto       38677 (45.6%)  OpenSim storage file. :func:`opynsim.read_sto`.
.vtp       14333 (16.9%)  VTK PolyData mesh file. :func:`opynsim.read_vtp`. Only vertices and faces are parsed. OpenSim <= 4.5 did not read any other data types. OPynSim does not plan on expanding VTP support beyond that.
.mot       4403 (5.2%)    OpenSim motion file. :func:`opynsim.read_mot`.
.xml       3846 (4.5%)    Generic XML file. Use Python's in-built XML parser or a third-party library.
.fig       2815 (3.3%)    MATLAB fig file. Use MATLAB to read it.
.mat       2502 (2.9%)    MATLAB matrix file. Use MATLAB to read it.
.osim      1949 (2.3%)    OpenSim model file. :func:`opynsim.read_osim`. Currently cannot read OpenSim v1 or v2 files.
.trc       1927 (2.3%)    Mocap data file. :func:`opynsim.read_trc`.
.log       1849 (2.2%)    Plaintext log file. Use a text editor to read it.
.ply       1640 (1.9%)    Mesh file. Only used in one project. Use Blender/Meshlab to read it.
.m         1505 (1.8%)    MATLAB source code. Use MATLAB to read/run it.
.txt       1101 (1.3%)    Plaintext file. Use a text editor to read it.
.csv       1068 (1.3%)    Comma-Separated Values file. :func:`opynsim.read_csv`.
.stl       694 (0.8%)     Standard Triangle Language mesh file. :func:`opynsim.read_stl`.
.png       641 (0.8%)     Portable Network Graphics image file. :func:`opynsim.read_png`.
.h         634 (0.7%)     C/C++ source file. Use a suitable editor to read it.
.obj       566 (0.7%)     Wavefront OBJ mesh file. :func:`opynsim.read_obj`.
.ds_store  502 (0.6%)     macOS hidden metadata file. Can be ignored.
.pkl       415 (0.5%)     Python pickle file. Use Python's ``pickle`` module to read it.
.asc       309 (0.4%)     Plaintext ASCII file. Use a text editor to read it.
.jpg       303 (0.4%)     Joint Photographic Experts Group image file. :func:`opynsim.read_jpeg`.
========== ============== =============================================================================
