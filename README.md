# OESelect

A high-performance molecular atom selection library for the [OpenEye Toolkits](https://www.eyesopen.com/), providing
PyMOL-compatible selection syntax for both C++ and Python.

OESelect parses human-readable selection strings like `protein and chain A` or `ligand around 5` into optimized
predicate trees that evaluate against OpenEye `OEMolBase` molecules. It integrates directly with the OpenEye atom
predicate interface, enabling use anywhere OEChem accepts `OEUnaryPredicate<OEAtomBase>`.

Try it out:
```bash
pip install oeselect
```

## Usage

Here are a few examples of using ``oeselect``.

### Python

```python
from openeye import oechem
from oeselect import OESelect, select, count, parse

mol = oechem.OEGraphMol()
with oechem.oemolistream("structure.pdb") as ifs:
    oechem.OEReadMolecule(ifs, mol)

# Select atoms matching a query (returns atom indices)
backbone_indices = select(mol, "backbone and chain A")

# Count matching atoms
num_waters = count(mol, "water")

# Use OESelect as a predicate with OpenEye functions
pred = OESelect(mol, "protein and chain A")
for atom in mol.GetAtoms(pred):
    print(atom.GetName())
num_selected = oechem.OECount(mol, pred)

# Parse and inspect a selection
sele = parse("protein and ligand around 5")
print(sele.ToCanonical())  # Normalized form
print(sele.IsEmpty())  # False
```

### C++

```cpp
#include <oeselect/oeselect.h>
#include <oechem.h>

// Parse once, reuse across molecules
auto sele = OESel::OESelection::Parse("protein and not backbone");

// Create a molecule-bound evaluator
OESel::OESelect sel(mol, sele);

// Compatible with OEChem's predicate interface
for (auto atom = mol.GetAtoms(sel); atom; ++atom) {
    std::cout << atom->GetName() << "\n";
}

// Or use OEChem counting functions directly
unsigned int n = OEChem::OECount(mol, sel);
```

`OESelection` objects are immutable and safe to share across threads. Each thread should create its own `OESelect`
instance, which maintains a per-molecule evaluation context with lazy-initialized caches.

## Residue Selectors

In addition to PyMOL-style selection strings, OESelect provides residue selectors for workflows that need stable
residue-level identifiers. A `Selector` identifies one residue using the format `NAME:NUMBER:ICODE:CHAIN`, such as
`ALA:123: :A`. The insertion-code field is a single space for residues without an insertion code.

Use selector helpers when you want to extract the residues matched by a normal selection, store those residue IDs, and
later reapply them as an OpenEye atom predicate.

Selector sets are returned in selector order: chain, residue number, insertion code, then residue name.

### Python

```python
from oeselect import (
    OESelect,
    OEResidueSelector,
    Selector,
    get_selector_string,
    mol_to_selector_set,
    parse,
    selector_set,
)

#################################################################
# Example 1: Using an existing selection
sele = OESelect(mol, "ligand around 5")
selectors = selector_set(sele)
for s in selectors:
    print(s.ToString())

#################################################################
# Example 2: Using a molecule and selection
selectors = selector_set(mol, "ligand around 5")
for s in selectors:
    print(s.ToString())

#################################################################
# Example 3: Parse from strings
selectors = selector_set("ALA:123: :A,GLY:124: :A")
for s in selectors:
    print(s.ToString())

# Alternative
s = Selector.FromString("ALA:123: :A")
print(s.name, s.residue_number, s.chain)

#################################################################
# Example 4: Use selectors as a predicate

# Match all atoms that belong to the selected residues.
residue_pred = OEResidueSelector(selectors)
for atom in mol.GetAtoms(residue_pred):
    print(atom.GetName())

#################################################################
# Example 5: Selectors from an entire molecule
all_residues = mol_to_selector_set(mol)
```

### C++

```cpp
#include <oeselect/oeselect.h>

// Extract Selector objects for residues with atoms matching a selection.
std::set<OESel::Selector> active_site =
    OESel::selector_set(mol, "ligand around 5");

// Existing parsed selections are accepted too.
OESel::OESelection selection = OESel::OESelection::Parse("ligand around 5");
std::set<OESel::Selector> same_selectors =
    OESel::selector_set(mol, selection);

// Molecule-bound OESelect objects are accepted too.
OESel::OESelect selector(mol, "ligand around 5");
std::set<OESel::Selector> selector_residues =
    OESel::selector_set(selector);

// Parse one or more selectors. Commas, semicolons, tabs, and newlines are accepted.
std::set<OESel::Selector> selectors =
    OESel::parse_selector_set("ALA:123: :A,GLY:124: :A");

// Use selectors as an OpenEye-compatible atom predicate.
OESel::OEResidueSelector residue_sel(active_site);
for (auto atom = mol.GetAtoms(residue_sel); atom; ++atom) {
    std::cout << atom->GetName() << "\n";
}

// Convert atoms or residues back to selector strings.
OESel::Selector selector = OESel::Selector::FromString("ALA:123: :A");
std::string residue_id = selector.ToString();

auto atom = mol.GetAtoms();
if (atom) {
    std::string atom_selector = OESel::get_selector_string(*atom);
}
```

## Selection Language Reference

### Atom Properties

| Keyword             | Syntax                        | Examples                                |
|---------------------|-------------------------------|-----------------------------------------|
| `name`              | Atom name (glob patterns)     | `name CA`, `name C*`, `name CA+CB+N`    |
| `resn`              | Residue name (glob patterns)  | `resn ALA`, `resn AL*`                  |
| `resi`              | Residue number                | `resi 42`, `resi 1-100`, `resi > 50`    |
| `chain`             | Chain identifier              | `chain A`                               |
| `elem`              | Element symbol                | `elem C`, `elem Fe`                     |
| `index`             | Atom index (zero-based)       | `index 0`, `index 0-99`, `index >= 100` |
| `id` / `serial`     | Atom serial number            | `id 42`, `id 1-100`, `id > 50`          |
| `alt`               | Alternate location            | `alt A`                                 |
| `b` / `bfactor`     | B-factor (temperature factor) | `b > 30.0`, `b 20.0-50.0`               |
| `frag` / `fragment` | Fragment number               | `frag 1`, `frag 1-3`                    |

### Component Types

| Keyword            | Description                                                |
|--------------------|------------------------------------------------------------|
| `protein`          | Atoms in standard amino acid residues                      |
| `ligand`           | Atoms in unrecognized residues (default classification)    |
| `water`            | Water molecules (HOH, WAT, H2O, and variants)              |
| `solvent`          | Water and common solvents (DMS, GOL, PEG, and others)      |
| `organic`          | Carbon-containing atoms excluding protein and nucleic acid |
| `backbone` / `bb`  | Protein backbone atoms (N, CA, C, O)                       |
| `sidechain` / `sc` | Protein sidechain atoms (excludes N, CA, C, O, OXT)        |
| `metal` / `metals` | Metal ions (Li, Na, Mg, K, Ca, Fe, Zn, Cu, and others)     |
| `capping` / `caps` | Terminal capping groups (ACE, NME)                          |

### Atom Types

| Keyword                                       | Description                            |
|-----------------------------------------------|----------------------------------------|
| `heavy`                                       | Non-hydrogen atoms (atomic number > 1) |
| `hydrogen` / `h`                              | All hydrogen atoms                     |
| `polar_hydrogen` / `polar_hydrogens` / `polarh` | Hydrogens bonded to N, O, or S         |
| `nonpolar_hydrogen` / `nonpolar_hydrogens` / `apolarh` | Hydrogens bonded to C                  |

### Secondary Structure

| Keyword | Description                           |
|---------|---------------------------------------|
| `helix` | Alpha helix residues                  |
| `sheet` | Beta sheet residues                   |
| `turn`  | Turn residues                         |
| `loop`  | Residues not in helix, sheet, or turn |

### Distance Operators

| Keyword             | Description                                                            |
|---------------------|------------------------------------------------------------------------|
| `<sele> around <r>` | Atoms within *r* angstroms of reference selection, excluding reference |
| `<sele> expand <r>` | Atoms within *r* angstroms of reference selection, including reference |
| `<sele> beyond <r>` | Atoms farther than *r* angstroms from reference selection              |

### Expansion Operators

| Keyword          | Description                                              |
|------------------|----------------------------------------------------------|
| `byres <sele>`   | Expand to complete residues containing any matching atom |
| `bychain <sele>` | Expand to complete chains containing any matching atom   |

### Logical Operators

Operator precedence from highest to lowest: `not`, `and`, `or`, `xor`. Parentheses override precedence.

| Operator | Aliases | Description         |
|----------|---------|---------------------|
| `not`    | `!`     | Logical negation    |
| `and`    | `&`     | Logical conjunction |
| `or`     | `\|`    | Logical disjunction |
| `xor`    | `^`     | Exclusive or        |

### Special Keywords

| Keyword | Description        |
|---------|--------------------|
| `all`   | Matches every atom |
| `none`  | Matches no atoms   |

### Hierarchical Macro Syntax

The `//chain/resi/name` syntax provides compact selections. Omitted fields act as wildcards:

```
//A//CA      chain A, any residue, atom name CA
//A/42/      chain A, residue 42, all atoms
///100/      any chain, residue 100, all atoms
//A/42/CA    chain A, residue 42, atom name CA
```

### Multi-value Syntax

The `+` separator allows matching multiple values in a single keyword:

```
name CA+CB+N       atoms named CA, CB, or N    equal to: (name CA or name CB or name N)
resn ALA+GLY+VAL   residues ALA, GLY, or VAL   equal to: (resn ALA or resn GLY or resn VAL)
```

## License

MIT License. See [LICENSE](LICENSE) for details.
