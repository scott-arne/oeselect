"""
OESelect - PyMOL-style selection language for OpenEye Toolkits

This package provides a fast C++ implementation of a PyMOL-compatible
selection language for use with OpenEye molecular modeling tools.

Install with: ``pip install oeselect``

Example usage::

    from openeye import oechem
    from oeselect import select, count, parse

    # Create a molecule
    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")

    # Select atoms by name
    indices = select("name CA", mol)

    # Count atoms matching a selection
    num_carbons = count("elem C", mol)

    # Parse and validate a selection string
    sele = parse("protein and chain A")
    print(sele.ToCanonical())

Supported selection keywords:

Atom properties:
    - name <pattern>: Atom name (supports wildcards * and ?)
    - resn <name>: Residue name (ALA, GLY, etc.)
    - resi <number>: Residue number (supports ranges like 1-10 and comparisons like > 50)
    - chain <id>: Chain identifier
    - elem <symbol>: Element symbol (C, N, O, etc.)
    - index <number>: Atom index (supports ranges and comparisons)

Component types:
    - protein: Protein atoms
    - ligand: Small molecule ligand atoms
    - water: Water molecules
    - solvent: Solvent molecules (water + common solvents)
    - organic: Small organic molecules
    - backbone / bb: Protein backbone atoms (N, CA, C, O)
    - sidechain / sc: Protein sidechain atoms
    - metal / metals: Metal ions

Atom types:
    - heavy: Non-hydrogen atoms
    - hydrogen / h: Hydrogen atoms
    - polar_hydrogen / polarh: Polar hydrogens
    - nonpolar_hydrogen / apolarh: Non-polar hydrogens

Secondary structure:
    - helix: Alpha helix atoms
    - sheet: Beta sheet atoms
    - turn: Turn atoms
    - loop: Loop/coil atoms

Distance-based:
    - around <radius> <selection>: Atoms within radius of selection
    - xaround <radius> <selection>: Same as around, excluding reference atoms
    - beyond <radius> <selection>: Atoms outside radius of selection

Expansion:
    - byres <selection>: Expand to complete residues
    - bychain <selection>: Expand to complete chains

Logical operators:
    - and / &: Logical AND
    - or / |: Logical OR
    - not / !: Logical NOT
    - xor / ^: Logical XOR

Special:
    - all: All atoms
    - none: No atoms
    - (parentheses): Grouping

Multi-value syntax:
    - name CA+CB+N: Equivalent to name CA or name CB or name N

Hierarchical macro syntax:
    - //chain/resi/name: e.g., //A/100/CA
"""

import warnings

# Version info
__version__ = "1.0.0"
__version_info__ = (1, 0, 0)


def _check_openeye_version():
    """Check that the OpenEye version matches what was used at build time."""
    try:
        from . import _build_info
    except ImportError:
        # Build info not available (development install)
        return

    # Only check if using shared/dynamic linking
    if getattr(_build_info, 'OPENEYE_LIBRARY_TYPE', 'STATIC') != 'SHARED':
        return

    build_version = getattr(_build_info, 'OPENEYE_BUILD_VERSION', None)
    if not build_version:
        return

    try:
        from openeye import oechem
        # Get runtime toolkit version using the official API
        runtime_version = oechem.OEToolkitsGetRelease()
        if runtime_version and build_version:
            # Compare major.minor.patch versions (e.g., "2025.2.1")
            build_parts = build_version.split('.')[:3]
            runtime_parts = runtime_version.split('.')[:3]
            if build_parts != runtime_parts:
                warnings.warn(
                    f"OpenEye version mismatch: oeselect was built with OpenEye Toolkits {build_version} "
                    f"but runtime has OpenEye Toolkits {runtime_version}. "
                    f"This may cause compatibility issues. Please ensure openeye-toolkits "
                    f"matches the version used to build oeselect.",
                    RuntimeWarning
                )
    except ImportError:
        warnings.warn(
            "openeye-toolkits package not found. "
            "This wheel requires openeye-toolkits to be installed. "
            "Install with: pip install openeye-toolkits",
            ImportWarning
        )


# Check OpenEye version on import
_check_openeye_version()

from .oeselect import (
    OESelection,
    EvaluateSelection,
    CountSelection,
    select,
    count,
    parse,
)

# Import PredicateType enum values
from .oeselect import (
    PredicateType_And,
    PredicateType_Or,
    PredicateType_Not,
    PredicateType_XOr,
    PredicateType_Name,
    PredicateType_Resn,
    PredicateType_Resi,
    PredicateType_Chain,
    PredicateType_Elem,
    PredicateType_Index,
    PredicateType_SecondaryStructure,
    PredicateType_Protein,
    PredicateType_Ligand,
    PredicateType_Water,
    PredicateType_Solvent,
    PredicateType_Organic,
    PredicateType_Backbone,
    PredicateType_Metal,
    PredicateType_Heavy,
    PredicateType_Hydrogen,
    PredicateType_PolarHydrogen,
    PredicateType_NonpolarHydrogen,
    PredicateType_ByRes,
    PredicateType_ByChain,
    PredicateType_Around,
    PredicateType_XAround,
    PredicateType_Box,
    PredicateType_XBox,
    PredicateType_Beyond,
    PredicateType_Helix,
    PredicateType_Sheet,
    PredicateType_Turn,
    PredicateType_Loop,
    PredicateType_True,
    PredicateType_False,
)

# Create a namespace for PredicateType enum
class PredicateType:
    """Enum-like class for predicate types."""
    And = PredicateType_And
    Or = PredicateType_Or
    Not = PredicateType_Not
    XOr = PredicateType_XOr
    Name = PredicateType_Name
    Resn = PredicateType_Resn
    Resi = PredicateType_Resi
    Chain = PredicateType_Chain
    Elem = PredicateType_Elem
    Index = PredicateType_Index
    SecondaryStructure = PredicateType_SecondaryStructure
    Protein = PredicateType_Protein
    Ligand = PredicateType_Ligand
    Water = PredicateType_Water
    Solvent = PredicateType_Solvent
    Organic = PredicateType_Organic
    Backbone = PredicateType_Backbone
    Metal = PredicateType_Metal
    Heavy = PredicateType_Heavy
    Hydrogen = PredicateType_Hydrogen
    PolarHydrogen = PredicateType_PolarHydrogen
    NonpolarHydrogen = PredicateType_NonpolarHydrogen
    ByRes = PredicateType_ByRes
    ByChain = PredicateType_ByChain
    Around = PredicateType_Around
    XAround = PredicateType_XAround
    Box = PredicateType_Box
    XBox = PredicateType_XBox
    Beyond = PredicateType_Beyond
    Helix = PredicateType_Helix
    Sheet = PredicateType_Sheet
    Turn = PredicateType_Turn
    Loop = PredicateType_Loop
    True_ = PredicateType_True
    False_ = PredicateType_False

__all__ = [
    "__version__",
    "__version_info__",
    "OESelection",
    "PredicateType",
    "EvaluateSelection",
    "CountSelection",
    "select",
    "count",
    "parse",
]
