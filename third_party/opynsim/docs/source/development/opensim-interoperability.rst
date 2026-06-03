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
    should them, where applicable. OPynSim ships a different API
    that is specifically catered for simpler use-cases and Python
    interoperability.


File Format Interoperability
----------------------------

It's important that OPynSim supports reading and writing commonly-used
data formats used in the OpenSim ecosystem, not just ``.osim`` model
files.

To get an idea of the distribution of data files in the OpenSim
ecosystem, 226 OpenSim studies were collected from SimTK.org. Their
contents were scanned for file extensions to find the most common
file formats used by researchers. The table below contains the top
21 (64572, 97 %) most-used formats in the collection, with comments.
2754 (3 %) of those files are removed from the table because they
were found to be macOS metadata files.

OPynSim currently provides parsers for around 75 % of the collection. Python
itself provides parsers for 5 % (``.xml``, ``.pkl``). Proprietary MATLAB-specific
files account for 8 %. The rest are either human-readable plaintext
(4.6 %; ``.log``, ``.txt``, ``.asc``), or are easily converted with
third-party software (1.9 %, ``.ply``). Which is to say, if you combine
OPynSim with the Python standard library, ignore MATLAB files, ignore
macOS metadata files, and edit plaintext documents with a text
editor then it's possible to parse >95 % of the collection.

========== ============== =============================================================================
Format     Count          Comments
========== ============== =============================================================================
.sto       36718 (45.6%)  OpenSim storage file. :func:`opynsim.read_sto`. Can parse 36711. 7 failures are malformed or contain too many data columns in one row.
.vtp       13708 (16.9%)  VTK PolyData mesh file. :func:`opynsim.read_vtp`. Can parse 13233. 475 use unsupported appended binary data (415), raw binary data (30), or other unsupported flags (30). Only vertices and faces are parsed (OpenSim legacy behavior).
.mot       4333 (5.2%)    OpenSim motion file. :func:`opynsim.read_mot`. Can parse 4320. 13 failures are mostly old SIMM files that use a ``range`` key, rather than a ``time`` column.
.xml       3846 (4.5%)    Generic XML file. Use Python's in-built XML parser or a third-party XML parser.
.fig       2815 (3.3%)    MATLAB fig file. Use MATLAB to read it.
.mat       2502 (2.9%)    MATLAB matrix file. Use MATLAB to read it.
.osim      1870 (2.3%)    OpenSim model file. :func:`opynsim.read_osim`. Can parse 1718. 152 failures require investigation, probably older v1 or v2 files.
.trc       1913 (2.3%)    Track row column file (motion data). :func:`opynsim.read_trc`. Can parse 1803. Failures require investigation.
.log       1849 (2.2%)    Plaintext log file. Use a text editor to read it.
.ply       1640 (1.9%)    Polygon file format (mesh data). Use Blender/Meshlab to read it.
.m         1505 (1.8%)    MATLAB source code. Use MATLAB to read/run it.
.txt       1101 (1.3%)    Plaintext file. Use a text editor to read it.
.csv       1067 (1.3%)    Comma-Separated Values file. Use Python's in-built CSV parser or :func:`opynsim.read_csv` (experimental). Beware, :func:`opynsim.read_csv` can only parse 16 of them (!) because it currently only supports numeric columns.
.stl       334 (0.8%)     Standard Triangle Language mesh file. :func:`opynsim.read_stl`. Can parse all of them.
.png       628 (0.8%)     Portable Network Graphics image file. :func:`opynsim.read_png`. Can parse all of them.
.h         634 (0.7%)     C/C++ source file. Use a text editor to read it.
.obj       564 (0.7%)     Wavefront OBJ mesh file. :func:`opynsim.read_obj`. Can parse 100 % of them.
.ds_store  502 (0.6%)     macOS hidden metadata file. Can be ignored.
.pkl       415 (0.5%)     Python pickle file. Use Python's ``pickle`` module to read it.
.asc       309 (0.4%)     Plaintext ASCII file. Use a text editor to read it.
.jpg       303 (0.4%)     Joint Photographic Experts Group image file. :func:`opynsim.read_jpg`. Can parse all of them.
.jpeg      11             Joint Photographic Experts Group image file. :func:`opynsim.read_jpeg`. Can parse all of them.
========== ============== =============================================================================
