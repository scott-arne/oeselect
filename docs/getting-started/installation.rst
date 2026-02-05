Installation
============

OESelect can be installed as a C++ library or Python package.

Prerequisites
-------------

- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: 3.16 or later
- **OpenEye Toolkit**: Licensed OpenEye OEChem library (C++ SDK for building, Python package for runtime)
- **Python**: 3.10 or later (for Python packages)
- **SWIG**: 4.0 or later (for building Python bindings)

C++ Library
-----------

Build from source using CMake:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/your-org/oeselect.git
   cd oeselect

   # Configure and build
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DOPENEYE_ROOT=/path/to/openeye/sdk
   cmake --build build --parallel

   # Run tests
   cd build && ctest --output-on-failure

   # Install (optional)
   cmake --install build --prefix /usr/local

CMake Options
^^^^^^^^^^^^^

.. list-table::
   :widths: 30 15 55
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - ``OESELECT_BUILD_TESTS``
     - ON
     - Build unit tests
   * - ``OESELECT_BUILD_PYTHON``
     - ON
     - Build Python bindings
   * - ``OPENEYE_ROOT``
     - (required)
     - Path to OpenEye C++ SDK

Python Package (pip)
--------------------

The recommended way to install OESelect for Python is via pip:

.. code-block:: bash

   # Install OpenEye Toolkits first
   pip install openeye-toolkits

   # Install OESelect (auto-selects correct binary for your OpenEye version)
   pip install oeselect

The ``oeselect`` metapackage automatically installs the correct ``oeselect-lib``
binary wheel matching your installed OpenEye Toolkits version.

Verify Installation
^^^^^^^^^^^^^^^^^^^

.. code-block:: python

   from oeselect import parse, select, count

   # Parse a selection
   sele = parse("protein and chain A")
   print(sele.ToCanonical())

   # Use with OpenEye molecule
   from openeye import oechem
   mol = oechem.OEGraphMol()
   oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")

   # Select carbon atoms
   carbons = select("elem C", mol)
   print(f"Found {len(carbons)} carbon atoms")

Building from Source (Python)
-----------------------------

If no pre-built wheel is available for your environment, build from source:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/your-org/oeselect.git
   cd oeselect

   # Build wheel (requires OpenEye C++ SDK)
   python scripts/build_python.py --openeye-root /path/to/openeye/sdk

   # Install the built wheel
   pip install dist/oeselect_lib-*.whl

Troubleshooting
---------------

OpenEye License
^^^^^^^^^^^^^^^

Ensure your OpenEye license is configured:

.. code-block:: bash

   export OE_LICENSE=/path/to/oe_license.txt

Import Errors
^^^^^^^^^^^^^

If Python cannot find the ``oeselect`` module, verify:

1. OpenEye Toolkits are installed: ``pip install openeye-toolkits``
2. The ``oeselect-lib`` wheel matches your OpenEye version
3. On Linux, OpenEye libraries are accessible (``LD_LIBRARY_PATH``)

Version Mismatch
^^^^^^^^^^^^^^^^

If you see a version mismatch error:

.. code-block:: bash

   # Reinstall with the correct version
   pip uninstall oeselect-lib
   pip install oeselect-lib==1.0.0+oe<your-openeye-version>

   # Or reinstall the metapackage
   pip install --force-reinstall oeselect
