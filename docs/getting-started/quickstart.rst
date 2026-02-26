Quick Start
===========

This guide shows how to use OESelect in both C++ and Python.

Selection Syntax
----------------

OESelect uses PyMOL-compatible selection syntax:

.. list-table:: Atom Properties
   :header-rows: 1
   :widths: 30 70

   * - Selector
     - Description
   * - ``name CA``
     - Select atoms named "CA"
   * - ``name C*``
     - Wildcard pattern matching
   * - ``resn ALA``
     - Residue name
   * - ``resi 100``
     - Residue number
   * - ``resi 50-100``
     - Residue range
   * - ``chain A``
     - Chain identifier
   * - ``elem C``
     - Element symbol
   * - ``index < 100``
     - Atom index comparison

.. list-table:: Component Types
   :header-rows: 1
   :widths: 30 70

   * - Selector
     - Description
   * - ``protein``
     - Protein atoms
   * - ``ligand``, ``lig``
     - Ligand atoms
   * - ``water``
     - Water molecules
   * - ``solvent``
     - Solvent molecules
   * - ``backbone``, ``bb``
     - Backbone atoms (N, CA, C, O)
   * - ``metal``
     - Metal ions

.. list-table:: Atom Types
   :header-rows: 1
   :widths: 30 70

   * - Selector
     - Description
   * - ``heavy``
     - Non-hydrogen atoms
   * - ``hydrogen``, ``h``
     - Hydrogen atoms

.. list-table:: Secondary Structure
   :header-rows: 1
   :widths: 30 70

   * - Selector
     - Description
   * - ``helix``
     - Alpha helix atoms
   * - ``sheet``
     - Beta sheet atoms
   * - ``turn``
     - Turn atoms
   * - ``loop``
     - Loop/coil atoms

.. list-table:: Logical Operators
   :header-rows: 1
   :widths: 30 70

   * - Operator
     - Description
   * - ``and``, ``&``
     - Logical AND
   * - ``or``, ``|``
     - Logical OR
   * - ``not``, ``!``
     - Logical NOT
   * - ``( )``
     - Grouping

.. list-table:: Special
   :header-rows: 1
   :widths: 30 70

   * - Selector
     - Description
   * - ``all``
     - All atoms
   * - ``none``
     - No atoms

Python Quick Start
------------------

Basic Selection
^^^^^^^^^^^^^^^

.. code-block:: python

    from oeselect import select, count
    from openeye import oechem

    # Create a molecule
    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

    # Select atoms by element
    carbon_indices = select(mol, "elem C")
    print(f"Carbon atom indices: {carbon_indices}")

    # Count matching atoms
    num_oxygens = count(mol, "elem O")
    print(f"Number of oxygens: {num_oxygens}")

    # Complex selections
    complex_oxygen_indices = select(mol, "heavy and not elem C")
    print(f"Oxygen atom indices: {complex_oxygen_indices}")

Outputs:

.. code-block:: text

    Carbon atom indices: [0, 1, 4, 5, 6, 7, 8, 9, 10]
    Number of oxygens: 4
    Oxygen atom indices: [2, 3, 11, 12]

Working with Proteins
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: python

    from openeye import oechem
    from oeselect import OESelect, select, count

    # Load a protein structure
    mol = oechem.OEGraphMol()
    with oechem.oemolistream("tests/assets/9Q03.cif") as ifs:
        oechem.OEReadMolecule(ifs, mol)

    # Get the ligand
    lig = oechem.OEGraphMol()
    lig_sel = OESelect(mol, "ligand")
    oechem.OESubsetMol(lig, mol, lig_sel)

    print(f"Ligand SMILES: {oechem.OEMolToSmiles(lig)}")

    # Select backbone atoms in chain A
    bb_chain_a = select(mol, "backbone and chain A")

    # Select all non-water heavy atoms
    heavy_non_water = select(mol, "heavy and not water")

    # Count residues in helix
    helix_count = count(mol, "helix and name CA")
    print(f"CA atoms in helix: {helix_count}")

Outputs:

.. code-block:: text

    Ligand SMILES: CC(C)(CCn1c2cc(ccc2n(c1=O)C)Nc3c(cnc(n3)N(C)C4CCN(CC4)c5ccc6c(c5)n(nc6[C@H]7CCC(=O)NC7=O)C)Cl)O
    CA atoms in helix: 220

C++ Quick Start
---------------

Basic Selection
^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <oeselect/oeselect.h>
   #include <openeye/oechem/oechem.h>
   #include <iostream>

   int main() {
       // Create a molecule
       OEChem::OEGraphMol mol;
       OEChem::OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O");

       // Parse a selection
       auto sele = OESel::OESelection::Parse("elem C");

       // Create evaluator bound to molecule
       OESel::OESelect sel(mol, sele);

       // Count matching atoms
       int count = 0;
       for (auto& atom : mol.GetAtoms(sel)) {
           count++;
       }
       std::cout << "Carbon atoms: " << count << "\n";

       return 0;
   }

Using with OpenEye Predicates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <oeselect/oeselect.h>
   #include <openeye/oechem/oechem.h>

   int main() {
       OEChem::OEGraphMol mol;
       // ... load molecule ...

       // OESelect is compatible with OEUnaryAtomPred
       OESel::OESelect sel(mol, "protein and not backbone");

       // Use with OEChem functions
       for (auto atom = mol.GetAtoms(sel); atom; ++atom) {
           std::cout << atom->GetName() << "\n";
       }

       // Or with standard predicates
       unsigned int num_selected = OEChem::OECount(mol, sel);

       return 0;
   }

Thread-Safe Usage
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <oeselect/oeselect.h>
   #include <thread>
   #include <vector>

   int main() {
       // Parse once (immutable, thread-safe)
       auto sele = OESel::OESelection::Parse("protein and chain A");

       std::vector<OEChem::OEGraphMol> molecules;
       // ... load molecules ...

       // Evaluate in parallel - each thread has its own OESelect context
       std::vector<std::thread> threads;
       for (auto& mol : molecules) {
           threads.emplace_back([&sele, &mol]() {
               OESel::OESelect sel(mol, sele);
               for (auto& atom : mol.GetAtoms(sel)) {
                   // Process atom
               }
           });
       }

       for (auto& t : threads) {
           t.join();
       }

       return 0;
   }

Selection Introspection
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <oeselect/oeselect.h>
   #include <iostream>

   int main() {
       auto sele = OESel::OESelection::Parse("protein and ligand around 5");

       // Get canonical form
       std::cout << sele.ToCanonical() << "\n";

       // Check predicate types
       if (sele.ContainsPredicate(OESel::PredicateType::PROTEIN)) {
           std::cout << "Contains protein predicate\n";
       }

       if (sele.ContainsPredicate(OESel::PredicateType::AROUND)) {
           std::cout << "Contains distance-based predicate\n";
       }

       return 0;
   }

Next Steps
----------

- :doc:`../cpp-api/library_root` - Full C++ API reference
- :doc:`../python-lib/index` - Python library documentation
