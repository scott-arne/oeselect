"""
OESelect - PyMOL-style selection language for OpenEye Toolkits

This package provides a fast C++ implementation of a PyMOL-compatible
selection language for use with OpenEye molecular modeling tools.

Install with: ``pip install oeselect``

Example usage::

    from openeye import oechem
    from oeselect import OESelect, select, count, parse

    # Create a molecule
    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")

    # Select atoms -- returns list of atom indices
    indices = select(mol, "name CA")

    # Count atoms matching a selection
    num_carbons = count(mol, "elem C")

    # Use OESelect as a predicate with OpenEye functions
    pred = OESelect(mol, "protein and chain A")
    for atom in mol.GetAtoms(pred):
        print(atom.GetName())

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
    - id / serial <number>: Atom serial number (supports ranges and comparisons)
    - alt <char>: Alternate location identifier
    - b / bfactor <number>: B-factor (supports float ranges and comparisons)
    - frag / fragment <number>: Fragment number (supports ranges and comparisons)

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
    - <selection> around <radius>: Atoms within radius of selection, excluding reference
    - <selection> expand <radius>: Atoms within radius of selection, including reference
    - <selection> beyond <radius>: Atoms outside radius of selection

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
__version__ = "1.0.1"
__version_info__ = (1, 0, 1)


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
    OESelect as _CppOESelect,
    Selector,
    OEResidueSelector as _CppOEResidueSelector,
    OEHasResidueName as _CppOEHasResidueName,
    OEHasAtomNameAdvanced as _CppOEHasAtomNameAdvanced,
    EvaluateSelection,
    CountSelection,
    ParseSelectorSet,
    MolToSelectorSet,
    StrSelectorSet,
    GetSelectorString,
    select,
    count,
    parse,
    str_selector_set,
    selector_set,
    mol_to_selector_set,
    get_selector_string,
)


try:
    from openeye import oechem as _oechem
    _OEUnaryAtomPred = _oechem.OEUnaryAtomPred
except ImportError:
    _OEUnaryAtomPred = object
    _oechem = None


class OESelect(_OEUnaryAtomPred):
    """Molecule-bound selection predicate compatible with OpenEye's predicate interface.

    Wraps the C++ OESelect evaluator and implements the OpenEye OEUnaryAtomPred
    protocol, allowing use with ``mol.GetAtoms(pred)`` and ``oechem.OECount(mol, pred)``.

    :param mol: An OpenEye OEMolBase object.
    :param sele: PyMOL-style selection string or OESelection object.

    Example::

        from openeye import oechem
        from oeselect import OESelect

        mol = oechem.OEGraphMol()
        oechem.OEReadMolecule(oechem.oemolistream("protein.pdb"), mol)

        pred = OESelect(mol, "protein and chain A")

        # Use with OpenEye atom iteration
        for atom in mol.GetAtoms(pred):
            print(atom.GetName())

        # Use with OpenEye counting
        num = oechem.OECount(mol, pred)
    """

    def __init__(self, mol, sele=None):
        _OEUnaryAtomPred.__init__(self)
        self._mol = mol
        if sele is None:
            self._cpp_select = _CppOESelect(mol, "all")
        elif isinstance(sele, str):
            self._cpp_select = _CppOESelect(mol, sele)
        elif isinstance(sele, OESelection):
            self._cpp_select = _CppOESelect(mol, sele)
        else:
            self._cpp_select = _CppOESelect(mol, str(sele))

    def __call__(self, atom):
        """Test if an atom matches the selection.

        :param atom: An OpenEye OEAtomBase object.
        :returns: True if the atom matches.
        """
        return self._cpp_select(atom)

    def __iter__(self):
        """Iterate over atoms matching the selection.

        :returns: Iterator of matching OEAtomBase objects.
        """
        return iter(self._mol.GetAtoms(self))

    def CreateCopy(self):
        """Create a copy for OpenEye compatibility."""
        copy = OESelect(self._mol, self._cpp_select.GetSelection())
        return copy.__disown__()

    @property
    def sele(self):
        """Access the underlying OESelection object."""
        return self._cpp_select.GetSelection()

    def __repr__(self):
        return f"OESelect('{self._cpp_select.GetSelection().ToCanonical()}')"


class OEResidueSelector(_OEUnaryAtomPred):
    """Predicate matching atoms by residue selector strings.

    Compatible with OpenEye's predicate interface for use with
    ``mol.GetAtoms(pred)`` and ``oechem.OECount(mol, pred)``.

    :param selector_str: Comma/semicolon/newline-separated selector strings
        in "NAME:NUMBER:ICODE:CHAIN" format.

    Example::

        sel = OEResidueSelector("ALA:123: :A,GLY:124: :A")
        for atom in mol.GetAtoms(sel):
            print(atom.GetName())
    """

    def __init__(self, selector_str):
        _OEUnaryAtomPred.__init__(self)
        if isinstance(selector_str, str):
            self._cpp_selector = _CppOEResidueSelector(selector_str)
        else:
            self._cpp_selector = _CppOEResidueSelector(selector_str)

    def __call__(self, atom):
        """Test if an atom belongs to one of the specified residues.

        :param atom: An OpenEye OEAtomBase object.
        :returns: True if the atom's residue matches.
        """
        return self._cpp_selector(atom)

    def CreateCopy(self):
        """Create a copy for OpenEye compatibility."""
        copy = OEResidueSelector.__new__(OEResidueSelector)
        _OEUnaryAtomPred.__init__(copy)
        copy._cpp_selector = _CppOEResidueSelector(self._cpp_selector)
        return copy.__disown__()


class OEHasResidueName(_OEUnaryAtomPred):
    """Match atoms by residue name with optional case/whitespace control.

    :param residue_name: The residue name to match.
    :param case_sensitive: If True, comparison is case-sensitive (default: False).
    :param whitespace: If True, whitespace is preserved (default: False).

    Example::

        pred = OEHasResidueName("ala")  # Matches "ALA", " ALA", etc.
    """

    def __init__(self, residue_name: str, case_sensitive: bool = False,
                 whitespace: bool = False):
        _OEUnaryAtomPred.__init__(self)
        self._cpp_pred = _CppOEHasResidueName(residue_name, case_sensitive, whitespace)
        self._residue_name = residue_name
        self._case_sensitive = case_sensitive
        self._whitespace = whitespace

    def __call__(self, atom):
        return self._cpp_pred(atom)

    def CreateCopy(self):
        copy = OEHasResidueName(self._residue_name, self._case_sensitive, self._whitespace)
        return copy.__disown__()


class OEHasAtomNameAdvanced(_OEUnaryAtomPred):
    """Match atoms by name with optional case/whitespace control.

    :param atom_name: The atom name to match.
    :param case_sensitive: If True, comparison is case-sensitive (default: False).
    :param whitespace: If True, whitespace is preserved (default: False).

    Example::

        pred = OEHasAtomNameAdvanced("ca")  # Matches "CA", " CA", etc.
    """

    def __init__(self, atom_name: str, case_sensitive: bool = False,
                 whitespace: bool = False):
        _OEUnaryAtomPred.__init__(self)
        self._cpp_pred = _CppOEHasAtomNameAdvanced(atom_name, case_sensitive, whitespace)
        self._atom_name = atom_name
        self._case_sensitive = case_sensitive
        self._whitespace = whitespace

    def __call__(self, atom):
        return self._cpp_pred(atom)

    def CreateCopy(self):
        copy = OEHasAtomNameAdvanced(self._atom_name, self._case_sensitive, self._whitespace)
        return copy.__disown__()

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
    PredicateType_Id,
    PredicateType_Alt,
    PredicateType_BFactor,
    PredicateType_Fragment,
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
    PredicateType_Expand,
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
    Id = PredicateType_Id
    Alt = PredicateType_Alt
    BFactor = PredicateType_BFactor
    Fragment = PredicateType_Fragment
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
    Expand = PredicateType_Expand
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
    "OESelect",
    "Selector",
    "OEResidueSelector",
    "OEHasResidueName",
    "OEHasAtomNameAdvanced",
    "PredicateType",
    "EvaluateSelection",
    "CountSelection",
    "select",
    "count",
    "parse",
    "str_selector_set",
    "selector_set",
    "mol_to_selector_set",
    "get_selector_string",
]
