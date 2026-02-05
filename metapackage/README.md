# oeselect

PyMOL-style selection language for OpenEye Toolkits.

## Installation

```bash
pip install oeselect
```

This single command:
1. Installs `openeye-toolkits` (if not already installed)
2. Detects your OpenEye Toolkits version
3. Automatically installs the matching `oeselect-lib` binary wheel

### Prerequisites

- **OpenEye License**: Set the `OE_LICENSE` environment variable

### Recommended: Pre-install OpenEye Toolkits

For best results, install `openeye-toolkits` first:

```bash
pip install openeye-toolkits
pip install oeselect
```

This ensures the correct binary is selected during installation.

## Usage

```python
from oeselect import select, count, parse
from openeye import oechem

# Create a molecule
mol = oechem.OEGraphMol()
oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

# Select atoms by element
carbon_indices = select("elem C", mol)
print(f"Carbon atoms: {carbon_indices}")

# Count atoms matching selection
num_oxygens = count("elem O", mol)
print(f"Number of oxygens: {num_oxygens}")

# Parse and validate selection string
sele = parse("protein and chain A")
print(f"Canonical form: {sele.ToCanonical()}")
```

### Supported Selection Keywords

**Atom properties:**
- `name <pattern>`: Atom name (supports wildcards * and ?)
- `resn <name>`: Residue name (ALA, GLY, etc.)
- `resi <number>`: Residue number (supports ranges like 1-10)
- `chain <id>`: Chain identifier
- `elem <symbol>`: Element symbol (C, N, O, etc.)
- `index <number>`: Atom index (supports ranges and comparisons)

**Component types:**
- `protein`: Protein atoms
- `ligand`: Small molecule ligand atoms
- `water`: Water molecules
- `solvent`: Solvent molecules
- `backbone` / `bb`: Protein backbone atoms
- `metal` / `metals`: Metal ions

**Atom types:**
- `heavy`: Non-hydrogen atoms
- `hydrogen` / `h`: Hydrogen atoms

**Secondary structure:**
- `helix`: Alpha helix atoms
- `sheet`: Beta sheet atoms
- `turn`: Turn atoms
- `loop`: Loop/coil atoms

**Logical operators:**
- `and` / `&`: Logical AND
- `or` / `|`: Logical OR
- `not` / `!`: Logical NOT

**Special:**
- `all`: All atoms
- `none`: No atoms
- Parentheses for grouping

## Package Architecture

The `oeselect` ecosystem consists of two packages:

1. **oeselect** (this package): A metapackage that:
   - Depends on `openeye-toolkits`
   - Dynamically determines the correct `oeselect-lib` version at install time
   - Re-exports all functionality from `oeselect-lib`

2. **oeselect-lib**: Pre-compiled binary wheels for each OpenEye Toolkits version
   - Linked against openeye-toolkits shared libraries
   - Version-tagged (e.g., `oeselect-lib==1.0.0+oe2025.2.1`)

### Why Version-Specific Binaries?

The binary code in `oeselect-lib` is compiled against specific OpenEye shared library
versions. Using a mismatched version can cause crashes or undefined behavior. The
dynamic dependency ensures you always get compatible binaries.

## Updating

When you update `openeye-toolkits`, reinstall oeselect:

```bash
pip install -U openeye-toolkits
pip install --force-reinstall oeselect
```

## Troubleshooting

### "oeselect-lib binary package is not installed"

The binary wasn't installed during `pip install oeselect`. This can happen if
`openeye-toolkits` wasn't installed beforehand.

Fix:
```bash
pip install openeye-toolkits
pip install --force-reinstall oeselect
```

### "oeselect-lib version mismatch"

Your `oeselect-lib` was built for a different OpenEye version.

Fix:
```bash
pip uninstall oeselect-lib
pip install oeselect-lib==1.0.0+oe<your-openeye-version>
```

### "No matching distribution found for oeselect-lib"

No pre-built wheel exists for your environment. Build from source:
```bash
git clone https://github.com/yourusername/oeselect
cd oeselect
python scripts/build_python.py --openeye-root /path/to/openeye/cpp/sdk
pip install dist/oeselect_lib-*.whl
```

## Building Wheels

To build `oeselect-lib` for a specific OpenEye version:

```bash
python scripts/build_python.py --openeye-root /path/to/openeye/cpp/sdk
```

The wheel will be named with the OpenEye version:
`oeselect_lib-1.0.0+oe2025.2.1-cp310-abi3-macosx_11_0_arm64.whl`

## License

MIT License (for oeselect wrapper code)

Note: This package requires OpenEye Toolkits, which requires a separate commercial license.
