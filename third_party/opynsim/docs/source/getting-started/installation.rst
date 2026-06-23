.. _installing-opynsim:

Installation
============

OPynSim is pre-built for its `supported platforms`_ and uploaded to `PyPi <https://pypi.org/project/opynsim/>`_, which
means you can easily install it with package managers such as `pip <https://pip.pypa.io>`_ and `uv <https://docs.astral.sh/uv/>`_
for your project's chosen Python distribution.

``opynsim`` is a fundamental package with minimal dependencies, making it compatible with a wide
variety of other python packages and alternative Python distributions (e.g. `Anaconda <https://anaconda.org/>`_ and
`Miniforge <https://github.com/conda-forge/miniforge>`_).

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


.. _supported-platforms:


Supported Platforms
-------------------

OPynSim contains a native Python extension module, so it's only compatible with
the platforms that it's built for. The following platforms are currently built
and deployed to `PyPi <https://pypi.org/project/opynsim/>`_ by the project:

+----------------+------------------------------------------------------------------------------------------------+------------------------+
| Python Version | Operating System                                                                               | Processor Architecture |
+================+================================================================================================+========================+
| ≥ 3.12         | Windows                                                                                        | amd64 (Intel/AMD)      |
+----------------+------------------------------------------------------------------------------------------------+------------------------+
| ≥ 3.12         | MacOS ≥ 14.5 (Sonoma)                                                                          | arm64 (Apple Silicon)  |
+----------------+------------------------------------------------------------------------------------------------+------------------------+
| ≥ 3.12         | Linux /w glibc 2.28 (e.g. Ubuntu >=20.04, Debian >=10, AlmaLinux >=8, RHEL >=8, OpenSUSE >=15) | amd64 (Intel/AMD)      |
+----------------+------------------------------------------------------------------------------------------------+------------------------+
