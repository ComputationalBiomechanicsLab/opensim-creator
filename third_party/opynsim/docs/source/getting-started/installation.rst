.. _installing-opynsim:

Installation
============

OPynSim is pre-built for its `supported platforms`_ and uploaded `PyPi <https://pypi.org/project/opynsim/>`_, which
means you can easily install it with package managers such as `pip <https://pip.pypa.io>`_ and `uv <https://docs.astral.sh/uv/>`_
for your project's chosen Python distribution.

``opynsim`` is a fundamental package that's designed to have almost no dependencies. This means
which you should be able to install it alongside a wide variety of other python packages
(e.g. ``opensim``) and alternative Python distributions (e.g. `Anaconda <https://anaconda.org/>`_
and `Miniforge <https://github.com/conda-forge/miniforge>`_).

pip
---

In a terminal, install ``opynsim`` with:

.. code:: bash

    pip install opynsim

This assumes you have already created/activated your chosen Python (>= v3.12)
environment. For example, like this:

.. code:: bash

    # Example virtual environment initialization on Linux/macOS
    python3 -m venv .venv
    source .venv/bin/activate
    pip install opynsim

uv
--

In a terminal, install ``opynsim`` with:

.. code:: bash

    uv install opynsim

This assumes you have already initialized/activated your chosen Python (>= v3.12)
environment . For example, like this:

.. code:: bash

    # Example uv environment setup
    uv init
    uv install opynsim


Anaconda/Miniconda/Miniforge
----------------------------

In a terminal, install ``opynsim`` with:

.. code:: bash

    pip install opynsim

This assumes you have already initialized/activated your chosen Python (>= v3.12)
environment . For example, like this:

.. code:: bash

    # Example conda environment setup
    conda create -n just-opynsim "python>=3.12"
    conda activate just-opynsim
    pip install opynsim

You can even install the ``opensim`` Python package alongside OPynSim:

.. code:: bash

    # Example that includes also installing `opensim` alongside OPynSim
    conda create -n opynsim-and-opensim "python==3.12"
    conda activate opynsim-and-opensim
    conda install -c opensim-org opensim
    pip install opynsim
    pip check  # Should be fine

**Note**: OPynSim's datastructures (e.g. ``opynsim.Model``) aren't compatible
with OpenSim's (e.g. ``opensim.Model``). You must serialize data whenever it's
passed between the two libraries.

.. _supported-platforms:

Supported Platforms
-------------------

OPynSim isn't a pure Python library. It contains a native Python extension module,
which means it's only compatible with the platforms that it's built for. The following
platforms are currently supported:

+----------------+------------------------------------------------------------------------------------------+------------------------+
| Python Version | Operating System                                                                         | Processor Architecture |
+================+==========================================================================================+========================+
| ≥ 3.12         | Windows                                                                                  | amd64 (Intel/AMD)      |
+----------------+------------------------------------------------------------------------------------------+------------------------+
| ≥ 3.12         | MacOS ≥ 14.5 (Sonoma)                                                                    | amd64 (Intel)          |
+----------------+------------------------------------------------------------------------------------------+------------------------+
| ≥ 3.12         | MacOS ≥ 14.5 (Sonoma)                                                                    | arm64 (Apple Silicon)  |
+----------------+------------------------------------------------------------------------------------------+------------------------+
| ≥ 3.12         | manylinux_2_34_x86_64:                                                                   | amd64 (Intel/AMD)      |
|                |                                                                                          |                        |
|                | - Debian 12+                                                                             |                        |
|                | - Ubuntu 21.10+                                                                          |                        |
|                | - Fedora 35+                                                                             |                        |
|                | - CentOS/RHEL 9+                                                                         |                        |
+----------------+------------------------------------------------------------------------------------------+------------------------+
