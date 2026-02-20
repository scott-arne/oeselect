Python Library (oeselect)
=========================

The ``oeselect`` Python package provides SWIG-generated bindings to the C++
OESelect library. It enables PyMOL-style atom selection on OpenEye molecules.

Overview
--------

The Python bindings mirror the C++ API, with Pythonic conveniences. Key
components include:

- :func:`parse` - Parse and validate selection strings
- :func:`select` - Select atoms from an OpenEye molecule
- :func:`count` - Count matching atoms
- :class:`OESelection` - Parsed selection object
- :class:`PredicateType` - Enum for predicate introspection

Installation
------------

Install via pip (recommended):

.. code-block:: bash

   pip install openeye-toolkits
   pip install oeselect

Or build from source:

.. code-block:: bash

   python scripts/build_python.py --openeye-root /path/to/openeye/sdk
   pip install dist/oeselect-*.whl

Basic Usage
-----------

.. code-block:: python

   from oeselect import parse, select, count
   from openeye import oechem

   # Create a molecule
   mol = oechem.OEGraphMol()
   oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

   # Select atoms
   carbons = select("elem C", mol)
   print(f"Carbon indices: {carbons}")

   # Count atoms
   num_heavy = count("heavy", mol)
   print(f"Heavy atoms: {num_heavy}")

   # Parse and validate
   sele = parse("protein and chain A")
   print(f"Canonical: {sele.ToCanonical()}")

API Reference
-------------

Functions
^^^^^^^^^

.. function:: parse(selection_str)

   Parse a selection string and return an OESelection object.

   :param selection_str: PyMOL-style selection string.
   :returns: OESelection object for validation and canonicalization.
   :raises ValueError: If the selection string is invalid.

   Example::

       sele = parse("name CA and chain A")
       print(sele.ToCanonical())

.. function:: select(selection_str, mol)

   Evaluate a selection string on an OpenEye molecule.

   :param selection_str: PyMOL-style selection string.
   :param mol: An OpenEye OEMolBase object (OEMol, OEGraphMol, etc.).
   :returns: List of atom indices matching the selection.

   Example::

       from openeye import oechem

       mol = oechem.OEGraphMol()
       oechem.OESmilesToMol(mol, "CC(=O)O")

       carbons = select("elem C", mol)

.. function:: count(selection_str, mol)

   Count atoms matching a selection in an OpenEye molecule.

   :param selection_str: PyMOL-style selection string.
   :param mol: An OpenEye OEMolBase object.
   :returns: Number of atoms matching the selection.

   Example::

       num_oxygens = count("elem O", mol)

OESelection Class
^^^^^^^^^^^^^^^^^

Represents a parsed, validated selection. Immutable and thread-safe.

**Class Methods:**

.. method:: OESelection.Parse(selection_str)

   Parse a selection string.

   :param selection_str: PyMOL-style selection string.
   :returns: OESelection object.
   :raises ValueError: If parsing fails.

**Instance Methods:**

.. method:: OESelection.ToCanonical()

   Get the canonical string representation of the selection.

   :returns: Canonicalized selection string.

.. method:: OESelection.ContainsPredicate(predicate_type)

   Check if the selection contains a specific predicate type.

   :param predicate_type: A PredicateType enum value.
   :returns: True if the predicate type is present.

.. method:: OESelection.IsEmpty()

   Check if the selection is empty (matches all atoms).

   :returns: True if the selection is empty.

PredicateType Enum
^^^^^^^^^^^^^^^^^^

Enum for introspecting selection predicates:

**Logical Operators:**

- ``PredicateType.And``
- ``PredicateType.Or``
- ``PredicateType.Not``
- ``PredicateType.XOr``

**Atom Properties:**

- ``PredicateType.Name``
- ``PredicateType.Resn``
- ``PredicateType.Resi``
- ``PredicateType.Chain``
- ``PredicateType.Elem``
- ``PredicateType.Index``
- ``PredicateType.SecondaryStructure``

**Component Types:**

- ``PredicateType.Protein``
- ``PredicateType.Ligand``
- ``PredicateType.Water``
- ``PredicateType.Solvent``
- ``PredicateType.Organic``
- ``PredicateType.Backbone``
- ``PredicateType.Metal``

**Atom Types:**

- ``PredicateType.Heavy``
- ``PredicateType.Hydrogen``
- ``PredicateType.PolarHydrogen``
- ``PredicateType.NonpolarHydrogen``

**Distance Operators:**

- ``PredicateType.Around``
- ``PredicateType.XAround``
- ``PredicateType.Beyond``

**Expansion Operators:**

- ``PredicateType.ByRes``
- ``PredicateType.ByChain``

**Secondary Structure:**

- ``PredicateType.Helix``
- ``PredicateType.Sheet``
- ``PredicateType.Turn``
- ``PredicateType.Loop``

**Constants:**

- ``PredicateType.True_``
- ``PredicateType.False_``

Selection Syntax
----------------

**Atom Properties:**

.. code-block:: text

   name CA          # Atom name (exact)
   name C*          # Glob pattern
   resn ALA         # Residue name
   resn GL*         # Residue name with wildcard
   resi 100         # Residue number
   resi 50-100      # Residue range
   resi > 50        # Residue comparison
   chain A          # Chain identifier
   elem C           # Element symbol
   index 0          # Atom index
   index < 100      # Index comparison

**Component Types:**

.. code-block:: text

   protein          # Protein atoms
   ligand           # Ligand atoms
   water            # Water molecules
   solvent          # Solvent molecules
   organic          # Small organic molecules
   backbone         # Backbone atoms (N, CA, C, O)
   bb               # Alias for backbone
   metal            # Metal ions

**Atom Types:**

.. code-block:: text

   heavy            # Non-hydrogen atoms
   hydrogen         # All hydrogens
   h                # Alias for hydrogen

**Secondary Structure:**

.. code-block:: text

   helix            # Alpha helix
   sheet            # Beta sheet
   turn             # Turn
   loop             # Loop/coil

**Logical Operators:**

.. code-block:: text

   protein and chain A
   elem C or elem N
   not water
   protein and not backbone
   (chain A or chain B) and helix

**Distance Operators:**

.. code-block:: text

   around 5 ligand       # Within 5A of ligand
   xaround 5 ligand      # Same, excluding ligand atoms
   beyond 10 protein     # More than 10A from protein

**Expansion Operators:**

.. code-block:: text

   byres around 5 ligand    # Complete residues within 5A of ligand
   bychain chain A          # Expand to complete chain

**Special:**

.. code-block:: text

   all              # All atoms
   none             # No atoms

**Multi-value Syntax:**

.. code-block:: text

   name CA+CB+N     # Equivalent to: name CA or name CB or name N
   resn ALA+GLY     # Multiple residue names

**Hierarchical Macro:**

.. code-block:: text

   //A/100/CA       # Chain A, residue 100, atom CA
   //A//            # All atoms in chain A
   ////CA           # All CA atoms

Error Handling
--------------

The bindings raise Python exceptions for errors:

.. code-block:: python

   from oeselect import parse

   try:
       sele = parse("invalid_keyword xyz")
   except ValueError as e:
       print(f"Parse error: {e}")

Thread Safety
-------------

- ``OESelection`` objects are immutable and thread-safe
- ``select()`` and ``count()`` can be called from multiple threads on different molecules
- For the same molecule, synchronize access externally

See Also
--------

- :doc:`../getting-started/quickstart` - Usage examples
- :doc:`../cpp-api/library_root` - C++ API (source of Python bindings)
