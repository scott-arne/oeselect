OESelect Documentation
======================

**PyMOL-style selection language for OpenEye Toolkits** - A high-performance
C++ library for parsing and evaluating molecular atom selections using familiar
PyMOL syntax.

.. note::

   This documentation covers the C++ core library and Python bindings (oeselect-lib).

Overview
--------

OESelect provides tools for:

- **Atom Selection**: Select atoms using intuitive PyMOL-like syntax
- **Selection Parsing**: Parse and validate selection strings
- **Predicate Evaluation**: High-performance atom matching with caching
- **OpenEye Integration**: Works seamlessly with OEChem molecules

Key Features
------------

- **Familiar Syntax**: Uses PyMOL selection language (``name CA``, ``chain A``, ``protein and not backbone``)
- **Fast Evaluation**: C++ core with per-molecule caching and spatial indexing
- **Thread-Safe**: Parsed selections are immutable and shareable across threads
- **Comprehensive Predicates**: Atom properties, component types, distance operators, and more
- **Python Bindings**: Full SWIG-based Python bindings with OpenEye integration

Quick Links
-----------

- :doc:`getting-started/installation` - Install OESelect
- :doc:`getting-started/quickstart` - Get started in 5 minutes
- :doc:`cpp-api/library_root` - C++ API Reference
- :doc:`python-lib/index` - Python Library Reference

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   getting-started/installation
   getting-started/quickstart

.. toctree::
   :maxdepth: 2
   :caption: C++ Library

   cpp-api/library_root

.. toctree::
   :maxdepth: 2
   :caption: Python Library

   python-lib/index

Indices and Tables
==================

* :ref:`genindex`
* :ref:`search`
