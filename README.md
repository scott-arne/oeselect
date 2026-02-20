# OESelect

A high-performance molecular atom selection library for the [OpenEye Toolkits](https://www.eyesopen.com/), providing PyMOL-compatible selection syntax for both C++ and Python.

OESelect parses human-readable selection strings like `protein and chain A` or `around 5 ligand` into optimized predicate trees that evaluate against OpenEye `OEMolBase` molecules. It integrates directly with the OpenEye atom predicate interface, enabling use anywhere OEChem accepts `OEUnaryPredicate<OEAtomBase>`.

## Features

OESelect provides a comprehensive selection language covering structural biology workflows. Selections are parsed once into immutable, thread-safe objects that can be shared across threads and applied to multiple molecules without re-parsing.

**Atom properties** such as name, residue name, residue number, chain, element, and atom index are supported with exact matching, glob-style wildcards (`name C*`), numeric ranges (`resi 50-100`), and comparisons (`index >= 100`).

**Component classification** automatically identifies molecular components from residue names. The Tagger system recognizes standard amino acids (including protonation variants like HID/HIE/HIP), nucleotides, cofactors (NAD, FAD, HEM, ATP, and others), water molecules, and common solvents. Unknown residues default to ligand. This powers the `protein`, `ligand`, `water`, `solvent`, `organic`, `backbone`, `sidechain`, and `metal` keywords.

**Distance-based selection** uses a nanoflann k-d tree spatial index for O(log n) radius queries. The `around`, `xaround` (exclusive), and `beyond` operators accept floating-point radii in angstroms and an arbitrary reference selection. Results are cached per-context, so repeated queries with the same parameters are O(1).

**Expansion operators** (`byres`, `bychain`) extend a selection to complete residues or chains, enabling queries like `byres around 5 ligand` to select entire residues near a binding site.

**Secondary structure** predicates (`helix`, `sheet`, `turn`, `loop`) read assignments from OpenEye's `OEResidue` data, and **atom type** predicates (`heavy`, `hydrogen`, `polar_hydrogen`, `nonpolar_hydrogen`) classify based on atomic number and bonding environment.

All predicates compose with boolean operators (`and`, `or`, `not`, `xor`) and parentheses, and a hierarchical macro syntax (`//A/42/CA`) provides compact shorthand for chain/residue/name combinations.

## Installation

### Python (recommended)

```bash
pip install openeye-toolkits
pip install oeselect
```

Requires Python 3.10 or later. Wheels are built with the Python stable ABI (abi3), so a single wheel works across Python 3.10 through 3.13. Pre-built wheels are available for Linux x86_64, Linux aarch64, and macOS (universal2).

### C++ (from source)

```bash
git clone https://github.com/scott-arne/oeselect.git
cd oeselect

cmake -B build -DCMAKE_BUILD_TYPE=Release -DOPENEYE_ROOT=/path/to/openeye/sdk
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

Requires a C++17 compiler, CMake 3.16+, and the OpenEye C++ SDK. Header-only dependencies (PEGTL and nanoflann) are fetched automatically via CMake FetchContent.

## Usage

### Python

```python
from openeye import oechem
from oeselect import OESelect, select, count, parse

mol = oechem.OEGraphMol()
ifs = oechem.oemolistream("structure.pdb")
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
sele = parse("protein and around 5 ligand")
print(sele.ToCanonical())       # Normalized form
print(sele.IsEmpty())           # False
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

`OESelection` objects are immutable and safe to share across threads. Each thread should create its own `OESelect` instance, which maintains a per-molecule evaluation context with lazy-initialized caches.

## Selection Language Reference

### Atom Properties

| Keyword | Syntax | Examples |
|---------|--------|----------|
| `name` | Atom name (glob patterns) | `name CA`, `name C*`, `name CA+CB+N` |
| `resn` | Residue name (glob patterns) | `resn ALA`, `resn AL*` |
| `resi` | Residue number | `resi 42`, `resi 1-100`, `resi > 50` |
| `chain` | Chain identifier | `chain A` |
| `elem` | Element symbol | `elem C`, `elem Fe` |
| `index` | Atom index (zero-based) | `index 0`, `index 0-99`, `index >= 100` |

### Component Types

| Keyword | Description |
|---------|-------------|
| `protein` | Atoms in standard amino acid residues |
| `ligand` | Atoms in unrecognized residues (default classification) |
| `water` | Water molecules (HOH, WAT, H2O, and variants) |
| `solvent` | Water and common solvents (DMS, GOL, PEG, and others) |
| `organic` | Carbon-containing atoms excluding protein and nucleic acid |
| `backbone` / `bb` | Protein backbone atoms (N, CA, C, O) |
| `sidechain` / `sc` | Protein sidechain atoms (excludes N, CA, C, O, OXT) |
| `metal` / `metals` | Metal ions (Li, Na, Mg, K, Ca, Fe, Zn, Cu, and others) |

### Atom Types

| Keyword | Description |
|---------|-------------|
| `heavy` | Non-hydrogen atoms (atomic number > 1) |
| `hydrogen` / `h` | All hydrogen atoms |
| `polar_hydrogen` / `polarh` | Hydrogens bonded to N, O, or S |
| `nonpolar_hydrogen` / `apolarh` | Hydrogens bonded to C |

### Secondary Structure

| Keyword | Description |
|---------|-------------|
| `helix` | Alpha helix residues |
| `sheet` | Beta sheet residues |
| `turn` | Turn residues |
| `loop` | Residues not in helix, sheet, or turn |

### Distance Operators

| Keyword | Description |
|---------|-------------|
| `around <r> <sele>` | Atoms within *r* angstroms of reference selection |
| `xaround <r> <sele>` | Same as `around`, but excludes the reference atoms |
| `beyond <r> <sele>` | Atoms farther than *r* angstroms from reference selection |

### Expansion Operators

| Keyword | Description |
|---------|-------------|
| `byres <sele>` | Expand to complete residues containing any matching atom |
| `bychain <sele>` | Expand to complete chains containing any matching atom |

### Logical Operators

Operator precedence from highest to lowest: `not`, `and`, `or`, `xor`. Parentheses override precedence.

| Operator | Aliases | Description |
|----------|---------|-------------|
| `not` | `!` | Logical negation |
| `and` | `&` | Logical conjunction |
| `or` | `\|` | Logical disjunction |
| `xor` | `^` | Exclusive or |

### Special Keywords

| Keyword | Description |
|---------|-------------|
| `all` | Matches every atom |
| `none` | Matches no atoms |

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
name CA+CB+N       atoms named CA, CB, or N
resn ALA+GLY+VAL   residues ALA, GLY, or VAL
```

## Architecture

OESelect is organized as a layered C++ library with Python bindings generated via SWIG.

```
                    +--------------------------+
                    |     Python API           |
                    |  select() count() parse()|
                    +-----------+--------------+
                                |
                    +-----------+--------------+
                    |     SWIG Bindings        |
                    |     _oeselect.so         |
                    +-----------+--------------+
                                |
  +-----------------------------+-------------------------------+
  |                        C++ Core Library                     |
  |                                                             |
  |  OESelection          OESelect            Context           |
  |  (immutable parse)    (mol-bound eval)    (caches/index)    |
  |       |                    |                   |            |
  |  +----+----+         +----+----+         +----+----+       |
  |  | Parser  |         |Predicate|         | Spatial |       |
  |  | (PEGTL) |         |  Tree   |         | Index   |       |
  |  +---------+         +---------+         |(nanoflann)|     |
  |                           |              +---------+       |
  |                      +----+----+                           |
  |                      | Tagger  |                           |
  |                      +---------+                           |
  +------------------------------+-----------------------------+
                                 |
                    +------------+-------------+
                    |     OpenEye OEChem       |
                    |  OEMolBase, OEAtomBase   |
                    +--------------------------+
```

The Parser converts selection strings into predicate trees using a PEGTL-based PEG grammar. Each predicate node implements `Evaluate(Context&, OEAtomBase&)`, and the Context provides lazy-initialized caching for distance queries, residue expansion, and chain expansion. The Tagger classifies atoms into molecular components on first access and stores results as OEChem generic data tags.

All PIMPL-based classes (OESelection, OESelect, Context, SpatialIndex) maintain binary compatibility across library versions.

## Project Structure

```
oeselect/
  include/oeselect/          C++ public headers
    oeselect.h               Umbrella header
    Selection.h              OESelection (immutable parsed selection)
    Selector.h               OESelect (molecule-bound evaluator)
    Context.h                Evaluation context with caching
    Predicate.h              Predicate base class and PredicateType enum
    Parser.h                 Selection string parser
    SpatialIndex.h           K-d tree spatial index
    Tagger.h                 Component classification
    Error.h                  SelectionError exception
    predicates/              Concrete predicate implementations
  src/                       C++ implementation files
  swig/                      SWIG interface and CMake config
  python/oeselect/           Python package
  scripts/
    build_python.py          Wheel building and packaging
    version.py               Cross-file version management
  tests/
    cpp/                     Google Test suite (137 tests)
    python/                  pytest suite
  cmake/                     CMake modules (FindOpenEye.cmake)
  docs/                      Sphinx + Doxygen documentation
  .github/workflows/         CI/CD (cross-platform wheel builds)
```

## Development

### Prerequisites

Building from source requires a C++17 compiler, CMake 3.16+, SWIG 4.0+, Python 3.10+, and the OpenEye C++ SDK. The project uses CMake presets for local development -- copy `CMakePresets.json` and adjust the `OPENEYE_ROOT` path to your SDK installation.

### Building and Testing

```bash
# Configure using preset
cmake --preset default

# Build all targets
cmake --build build --parallel

# Run C++ tests (137 tests across 10 categories)
ctest --test-dir build --output-on-failure

# Run Python tests
pytest tests/python -v
```

### Building Python Wheels

The `scripts/build_python.py` script handles the full wheel build pipeline, including OpenEye SDK discovery from CMake presets, SWIG generation, and platform-specific wheel repair (auditwheel on Linux, delocate on macOS).

```bash
python scripts/build_python.py --openeye-root /path/to/openeye/sdk --verbose
```

### Version Management

The `scripts/version.py` script keeps version numbers synchronized across the C++ header, CMakeLists.txt, pyproject.toml, Python `__init__.py`, and Sphinx `conf.py`.

```bash
python scripts/version.py get              # Display current version in all files
python scripts/version.py bump patch       # Increment patch version everywhere
python scripts/version.py sync 2.0.0 --yes # Set all locations to a specific version
```

### Building Documentation

```bash
pip install -r docs/requirements.txt
cd docs && make html
```

Documentation is built with Sphinx using the Read the Docs theme, with C++ API references generated from Doxygen via Breathe and Exhale.

## CI/CD

The GitHub Actions workflow builds wheels for three platforms on every push to `main`/`master` and on pull requests:

| Platform | Container | Architecture | Wheel Tag |
|----------|-----------|--------------|-----------|
| Linux | Rocky Linux 8 | x86_64 | manylinux_2_28_x86_64 |
| Linux | Rocky Linux 9 | aarch64 | manylinux_2_34_aarch64 |
| macOS | macos-latest | universal2 | macosx_11_0_universal2 |

Publishing to PyPI is triggered automatically when a version tag (`v*`) is pushed, using PyPI trusted publishing (no API tokens required).

The workflow requires three repository secrets: `GCP_WORKLOAD_IDENTITY_PROVIDER` and `GCP_SERVICE_ACCOUNT` for downloading the OpenEye C++ SDK from Google Cloud Storage, and `OE_LICENSE` containing the OpenEye license file contents.

## License

MIT License. See [LICENSE](LICENSE) for details.
