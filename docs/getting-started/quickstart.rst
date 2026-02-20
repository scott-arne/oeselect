Quick Start
===========

This guide shows how to use OESelect in both C++ and Python.

Selection Syntax
----------------

OESelect uses PyMOL-compatible selection syntax:

**Atom Properties:**

- ``name CA`` - Select atoms named "CA"
- ``name C*`` - Wildcard pattern matching
- ``resn ALA`` - Residue name
- ``resi 100`` - Residue number
- ``resi 50-100`` - Residue range
- ``chain A`` - Chain identifier
- ``elem C`` - Element symbol
- ``index < 100`` - Atom index comparison

**Component Types:**

- ``protein`` - Protein atoms
- ``ligand`` - Ligand atoms
- ``water`` - Water molecules
- ``solvent`` - Solvent molecules
- ``backbone`` or ``bb`` - Backbone atoms (N, CA, C, O)
- ``metal`` - Metal ions

**Atom Types:**

- ``heavy`` - Non-hydrogen atoms
- ``hydrogen`` or ``h`` - Hydrogen atoms

**Secondary Structure:**

- ``helix`` - Alpha helix atoms
- ``sheet`` - Beta sheet atoms
- ``turn`` - Turn atoms
- ``loop`` - Loop/coil atoms

**Logical Operators:**

- ``and`` or ``&`` - Logical AND
- ``or`` or ``|`` - Logical OR
- ``not`` or ``!`` - Logical NOT
- Parentheses for grouping

**Special:**

- ``all`` - All atoms
- ``none`` - No atoms

Python Quick Start
------------------

Basic Selection
^^^^^^^^^^^^^^^

.. code-block:: python

   from oeselect import parse, select, count
   from openeye import oechem

   # Create a molecule
   mol = oechem.OEGraphMol()
   oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

   # Select atoms by element
   carbon_indices = select(mol, "elem C")
   print(f"Carbon atoms: {carbon_indices}")

   # Count matching atoms
   num_oxygens = count(mol, "elem O")
   print(f"Number of oxygens: {num_oxygens}")

   # Complex selections
   heavy_atoms = select(mol, "heavy and not elem O")

Selection Parsing
^^^^^^^^^^^^^^^^^

.. code-block:: python

   from oeselect import parse, PredicateType

   # Parse and validate a selection string
   sele = parse("protein and chain A")

   # Get canonical form
   print(sele.ToCanonical())  # "(chain A and protein)"

   # Check predicate types
   if sele.ContainsPredicate(PredicateType.Protein):
       print("Selection includes protein predicate")

   if sele.ContainsPredicate(PredicateType.Chain):
       print("Selection includes chain predicate")

Working with Proteins
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: python

   from openeye import oechem
   from oeselect import OESelect, select, count

   # Load a protein structure
   mol = oechem.OEGraphMol()
   ifs = oechem.oemolistream("protein.pdb")
   oechem.OEReadMolecule(ifs, mol)

   # Select backbone atoms in chain A
   bb_chain_a = select(mol, "backbone and chain A")

   # Select all non-water heavy atoms
   heavy_non_water = select(mol, "heavy and not water")

   # Count residues in helix
   helix_count = count(mol, "helix and name CA")
   print(f"CA atoms in helix: {helix_count}")

   # Use OESelect as a predicate with OpenEye functions
   pred = OESelect(mol, "protein and chain A")
   for atom in mol.GetAtoms(pred):
       print(atom.GetName())
   num_selected = oechem.OECount(mol, pred)

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
       auto sele = OESel::OESelection::Parse("protein and around 5 ligand");

       // Get canonical form
       std::cout << sele.ToCanonical() << "\n";

       // Check predicate types
       if (sele.ContainsPredicate(OESel::PredicateType::Protein)) {
           std::cout << "Contains protein predicate\n";
       }

       if (sele.ContainsPredicate(OESel::PredicateType::Around)) {
           std::cout << "Contains distance-based predicate\n";
       }

       return 0;
   }

Next Steps
----------

- :doc:`../cpp-api/library_root` - Full C++ API reference
- :doc:`../python-lib/index` - Python library documentation
