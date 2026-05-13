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
    - polar_hydrogen / polar_hydrogens / polarh: Polar hydrogens
    - nonpolar_hydrogen / nonpolar_hydrogens / apolarh: Hydrogens bonded to carbon

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

import hashlib
import importlib.machinery
import importlib.util
import os
import re
import shutil
import sys
import warnings
from importlib import metadata
from pathlib import Path

# Version info
__version__ = "1.3.8"
__version_info__ = (1, 3, 8)

_OPENEYE_COMPAT_PRELOAD_PATHS: list[str] = []
_OPENEYE_COMPAT_EXTENSION_DIR: Path | None = None


def _user_cache_root():
    """Return the per-user cache root for OpenEye compatibility aliases."""
    cache_home = os.environ.get("XDG_CACHE_HOME")
    if cache_home:
        return Path(cache_home) / "oeselect"
    return Path.home() / ".cache" / "oeselect"


def _runtime_openeye_version():
    """Return the installed OpenEye toolkit distribution version if available."""
    try:
        return metadata.version("openeye-toolkits")
    except metadata.PackageNotFoundError:
        return "unknown"


def _cache_key(oe_lib_dir, expected_libs, build_version, runtime_version):
    """Build a stable cache key for one OpenEye runtime library set."""
    key_data = "\n".join(
        [
            os.path.realpath(oe_lib_dir),
            build_version or "unknown",
            runtime_version or "unknown",
            *sorted(expected_libs),
        ]
    )
    return hashlib.sha256(key_data.encode("utf-8")).hexdigest()[:16]


def _runtime_shared_library_names(lib_names):
    """Return filenames that can participate in runtime dynamic loading."""
    return [
        lib_name
        for lib_name in lib_names
        if ".so" in lib_name
        or lib_name.endswith(".dylib")
        or lib_name.endswith(".dll")
    ]


def _find_openeye_runtime_lib_dir(expected_libs=()):
    """Find the OpenEye runtime library directory without importing oechem."""
    search_locations = []
    openeye_module = sys.modules.get("openeye")
    openeye_path = getattr(openeye_module, "__path__", None)
    if openeye_path is not None:
        search_locations.extend(openeye_path)

    if not search_locations:
        try:
            openeye_spec = importlib.util.find_spec("openeye")
        except (ImportError, ValueError):
            openeye_spec = None
        if (
            openeye_spec is not None
            and openeye_spec.submodule_search_locations is not None
        ):
            search_locations.extend(openeye_spec.submodule_search_locations)

    expected_libs = set(_runtime_shared_library_names(expected_libs or ()))
    fallback_dir = None
    for package_root in search_locations:
        libs_root = Path(package_root) / "libs"
        if not libs_root.is_dir():
            continue

        # Importing openeye.libs eagerly imports oechem in some environments.
        # The runtime libraries are shipped below openeye/libs, so filesystem
        # discovery preserves the fresh-import condition.
        for root, _, files in os.walk(libs_root):
            file_set = set(files)
            if expected_libs and expected_libs.intersection(file_set):
                return root
            if fallback_dir is None and any(
                ".dylib" in lib_name or ".so" in lib_name or ".dll" in lib_name
                for lib_name in files
            ):
                fallback_dir = root

    return fallback_dir


def _library_family(lib_name):
    """Return the stable library family name for a versioned shared library."""
    match = re.match(r"(lib\w+?)(-[\d.]+)?(\.[\d.]*\w+)$", lib_name)
    if match is None:
        return None
    return match.group(1)


def _candidate_runtime_libraries(oe_lib_dir, expected_name):
    """Find runtime libraries with the same family as an expected filename."""
    family = _library_family(expected_name)
    if family is None:
        return []
    candidates = []
    for file_name in os.listdir(oe_lib_dir):
        candidate_path = os.path.join(oe_lib_dir, file_name)
        if not os.path.isfile(candidate_path):
            continue
        if file_name.startswith(f"{family}-") or file_name.startswith(f"{family}."):
            candidates.append(candidate_path)
    return sorted(candidates)


def _compatible_library_path(oe_lib_dir, expected_name):
    """Return a runtime library path and whether it needs an expected-name alias."""
    exact_path = os.path.join(oe_lib_dir, expected_name)
    if os.path.isfile(exact_path):
        return exact_path, False

    candidates = _candidate_runtime_libraries(oe_lib_dir, expected_name)
    if len(candidates) != 1:
        candidate_names = ", ".join(os.path.basename(path) for path in candidates)
        raise ImportError(
            f"Could not find a compatible OpenEye runtime library for "
            f"{expected_name!r} in {oe_lib_dir!r}. "
            f"Candidates: {candidate_names or 'none'}."
        )
    return candidates[0], True


def _ensure_cache_alias(cache_dir, expected_name, target_path):
    """Create or refresh an expected-name symlink in the user cache."""
    alias_path = cache_dir / expected_name
    if alias_path.is_symlink():
        if alias_path.resolve() == Path(target_path).resolve():
            return alias_path
        alias_path.unlink()
    elif alias_path.exists():
        raise ImportError(
            f"Cannot create OpenEye compatibility alias {alias_path}: "
            "a non-symlink file already exists at that path."
        )

    try:
        alias_path.symlink_to(target_path)
    except OSError as exc:
        raise ImportError(
            f"Could not create OpenEye compatibility alias "
            f"{alias_path} -> {target_path}: {exc}"
        ) from exc
    return alias_path


def _ensure_library_compat():
    """Prepare compatibility aliases when OpenEye library filenames drift.

    When oeselect is built with shared OpenEye libraries, the compiled extension
    records the exact versioned library filenames (e.g., liboechem-4.3.0.1.dylib).
    If the user upgrades openeye-toolkits, these filenames change (e.g., to
    liboechem-4.3.0.2.dylib) and the dynamic linker fails to load the extension.

    This function creates expected-name aliases in a user-writable cache instead
    of mutating the installed package directory. When aliases are needed, the
    extension is later loaded from the same cache directory so its $ORIGIN lookup
    can find those aliases.
    """
    global _OPENEYE_COMPAT_EXTENSION_DIR, _OPENEYE_COMPAT_PRELOAD_PATHS

    _OPENEYE_COMPAT_PRELOAD_PATHS = []
    _OPENEYE_COMPAT_EXTENSION_DIR = None

    try:
        from . import _build_info
    except ImportError:
        return False

    if getattr(_build_info, 'OPENEYE_LIBRARY_TYPE', 'STATIC') != 'SHARED':
        return False

    expected_libs = _runtime_shared_library_names(
        getattr(_build_info, 'OPENEYE_EXPECTED_LIBS', [])
    )
    if not expected_libs:
        return False

    oe_lib_dir = _find_openeye_runtime_lib_dir(expected_libs)
    if oe_lib_dir is None:
        return False

    if not os.path.isdir(oe_lib_dir):
        return False

    build_version = getattr(_build_info, 'OPENEYE_BUILD_VERSION', None)
    runtime_version = _runtime_openeye_version()
    cache_dir = (
        _user_cache_root()
        / "openeye-libs"
        / _cache_key(oe_lib_dir, expected_libs, build_version, runtime_version)
    )

    preload_paths = []
    needs_cached_origin = False
    for expected_name in expected_libs:
        actual_path, needs_alias = _compatible_library_path(oe_lib_dir, expected_name)
        if needs_alias:
            try:
                cache_dir.mkdir(parents=True, exist_ok=True)
            except OSError as exc:
                raise ImportError(
                    f"Could not create OpenEye compatibility cache directory "
                    f"{cache_dir}: {exc}"
                ) from exc
            alias_path = _ensure_cache_alias(cache_dir, expected_name, actual_path)
            preload_paths.append(str(alias_path))
            needs_cached_origin = True
        else:
            preload_paths.append(actual_path)

    _OPENEYE_COMPAT_PRELOAD_PATHS = preload_paths
    if needs_cached_origin:
        _OPENEYE_COMPAT_EXTENSION_DIR = cache_dir

    return needs_cached_origin


def _extension_suffixes():
    """Return extension-module suffixes for the active Python interpreter."""
    return tuple(importlib.machinery.EXTENSION_SUFFIXES)


def _find_extension_module_path(pkg_dir):
    """Find the installed _oeselect extension file."""
    for suffix in _extension_suffixes():
        candidate = Path(pkg_dir) / f"_oeselect{suffix}"
        if candidate.is_file():
            return candidate
    for candidate in Path(pkg_dir).glob("_oeselect*"):
        if candidate.is_file() and str(candidate).endswith(_extension_suffixes()):
            return candidate
    return None


def _copy_if_stale(source_path, target_path):
    """Copy a file into the cache when size or mtime changed."""
    if (
        target_path.exists()
        and target_path.stat().st_size == source_path.stat().st_size
        and target_path.stat().st_mtime_ns == source_path.stat().st_mtime_ns
    ):
        return
    shutil.copy2(source_path, target_path)


def _copy_package_shared_sidecars(pkg_dir, cache_dir, extension_path):
    """Copy package-local shared library sidecars needed by cached extension."""
    for candidate in Path(pkg_dir).iterdir():
        name = candidate.name
        if not candidate.is_file() or candidate == extension_path:
            continue
        if (
            ".so" not in name
            and not name.endswith(".dylib")
            and not name.endswith(".dll")
            and not name.endswith(".pyd")
        ):
            continue
        _copy_if_stale(candidate, cache_dir / name)


def _load_cached_extension_if_needed():
    """Load _oeselect from the cache when OpenEye aliases live there."""
    cache_dir = _OPENEYE_COMPAT_EXTENSION_DIR
    if cache_dir is None:
        return

    module_name = f"{__name__}._oeselect"
    if module_name in sys.modules:
        return

    pkg_dir = os.path.dirname(__file__)
    extension_path = _find_extension_module_path(pkg_dir)
    if extension_path is None:
        return

    cached_extension_path = cache_dir / extension_path.name
    try:
        cache_dir.mkdir(parents=True, exist_ok=True)
        _copy_if_stale(extension_path, cached_extension_path)
        _copy_package_shared_sidecars(pkg_dir, cache_dir, extension_path)
    except OSError as exc:
        raise ImportError(
            f"Could not prepare cached oeselect extension in {cache_dir}: {exc}"
        ) from exc

    spec = importlib.util.spec_from_file_location(module_name, cached_extension_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Could not create import spec for {cached_extension_path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    try:
        spec.loader.exec_module(module)
    except Exception:
        sys.modules.pop(module_name, None)
        raise


def _preload_shared_libs():
    """Preload OpenEye shared libraries so the C extension can find them.

    On Linux, the extension's RUNPATH (set at build time) normally handles
    dependency resolution, but preloading ensures libraries are available
    even if RUNPATH is stripped (e.g. by certain packaging tools).
    On macOS, @rpath references may not resolve without preloading.

    Only the libraries recorded in ``OPENEYE_EXPECTED_LIBS`` are loaded,
    and they are loaded with ``RTLD_GLOBAL`` so that cross-module C++
    symbol references resolve correctly. Loading the entire OpenEye
    library directory (which can contain 70+ unrelated shared objects)
    would pollute the global symbol namespace and cause segfaults in
    unrelated C extensions such as ``_sqlite3``.
    """
    import ctypes
    import sys
    if sys.platform not in ('linux', 'darwin'):
        return

    try:
        from . import _build_info
    except ImportError:
        return

    if getattr(_build_info, 'OPENEYE_LIBRARY_TYPE', 'STATIC') != 'SHARED':
        return

    expected_libs = _runtime_shared_library_names(
        getattr(_build_info, 'OPENEYE_EXPECTED_LIBS', [])
    )
    if not expected_libs:
        return

    oe_lib_dir = _find_openeye_runtime_lib_dir(expected_libs)
    if oe_lib_dir is None:
        return

    if not os.path.isdir(oe_lib_dir):
        return

    paths = _OPENEYE_COMPAT_PRELOAD_PATHS
    if not paths:
        paths = [
            os.path.join(oe_lib_dir, lib_name)
            for lib_name in expected_libs
            if os.path.exists(os.path.join(oe_lib_dir, lib_name))
        ]

    for path in paths:
        if os.path.exists(path) or os.path.islink(path):
            try:
                ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
            except OSError:
                pass


def _preload_bundled_libs():
    """Preload libraries bundled by auditwheel from the .libs directory.

    auditwheel repair bundles non-manylinux dependencies (e.g. libraries
    from FetchContent or system packages) into a ``<package>.libs/``
    directory next to the package. The bundled copies have hashed filenames
    and must be loaded before the C extension to satisfy its DT_NEEDED
    entries.

    Libraries may have inter-dependencies, so we do multiple passes
    until no new libraries can be loaded. Libraries are loaded without
    ``RTLD_GLOBAL`` to avoid polluting the global symbol namespace.
    """
    import sys
    if sys.platform != 'linux':
        return

    import ctypes
    pkg_name = __name__
    pkg_dir = os.path.dirname(os.path.abspath(__file__))
    site_dir = os.path.dirname(pkg_dir)
    for libs_name in (f'{pkg_name}.libs', f'.{pkg_name}.libs'):
        libs_dir = os.path.join(site_dir, libs_name)
        if not os.path.isdir(libs_dir):
            continue
        remaining = [
            os.path.join(libs_dir, f)
            for f in sorted(os.listdir(libs_dir))
            if '.so' in f
        ]
        while remaining:
            failed = []
            for lib_path in remaining:
                try:
                    ctypes.CDLL(lib_path)
                except OSError:
                    failed.append(lib_path)
            if len(failed) == len(remaining):
                break
            remaining = failed


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

    runtime_version = _runtime_openeye_version()
    if runtime_version == "unknown":
        warnings.warn(
            "openeye-toolkits package not found. "
            "This wheel requires openeye-toolkits to be installed. "
            "Install with: pip install openeye-toolkits",
            ImportWarning
        )
        return

    build_parts = build_version.split('.')[:2]
    runtime_parts = runtime_version.split('.')[:2]
    if build_parts != runtime_parts:
        warnings.warn(
            f"OpenEye version mismatch: oeselect was built with OpenEye Toolkits {build_version} "
            f"but runtime has OpenEye Toolkits {runtime_version}. "
            f"This may cause compatibility issues. Please ensure openeye-toolkits "
            f"matches the version used to build oeselect.",
            RuntimeWarning
        )


# Prepare compatibility aliases before loading the C extension
_ensure_library_compat()

# Preload OpenEye shared libraries (needed on Linux where RUNPATH may not
# include the OpenEye library directory after auditwheel repair)
_preload_shared_libs()

# Preload auditwheel-bundled libraries from .libs directory
_preload_bundled_libs()

# Load the extension from the alias cache when $ORIGIN must see cached aliases
_load_cached_extension_if_needed()

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
    parse_selector_set,
    mol_to_selector_set,
    str_selector_set,
    get_selector_string,
    select,
    count,
    parse,
    selector_set as _cpp_selector_set,
)


def _get_openeye_atom_predicate_base():
    """Load the OpenEye atom predicate base only when a predicate is instantiated."""
    from openeye import oechem

    return oechem.OEUnaryAtomPred


def _get_lazy_openeye_predicate_type(public_cls):
    """Create the OpenEye-backed subclass for a public predicate wrapper."""
    predicate_cls = getattr(public_cls, "_openeye_predicate_type", None)
    if predicate_cls is None:
        predicate_cls = type(
            public_cls.__name__,
            (public_cls, _get_openeye_atom_predicate_base()),
            {"__module__": public_cls.__module__},
        )
        public_cls._openeye_predicate_type = predicate_cls
    return predicate_cls


class OESelect:
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

    _openeye_predicate_type = None

    def __new__(cls, *args, **kwargs):
        if cls is OESelect:
            predicate_cls = _get_lazy_openeye_predicate_type(cls)
            return predicate_cls.__new__(predicate_cls)
        return super().__new__(cls)

    def __init__(self, mol, sele=None):
        _get_openeye_atom_predicate_base().__init__(self)
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


def selector_set(*args):
    """Create a set of Selector objects.

    Accepts either selector strings, a molecule plus selection, or a
    molecule-bound OESelect object.

    :param args: ``selector_str``, ``(mol, selection)``, or ``OESelect``.
    :returns: Set of Selector objects in selector order.
    """
    if len(args) == 1 and isinstance(args[0], OESelect):
        return _cpp_selector_set(args[0]._cpp_select)
    return _cpp_selector_set(*args)


class OEResidueSelector:
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

    _openeye_predicate_type = None

    def __new__(cls, *args, **kwargs):
        if cls is OEResidueSelector:
            predicate_cls = _get_lazy_openeye_predicate_type(cls)
            return predicate_cls.__new__(predicate_cls)
        return super().__new__(cls)

    def __init__(self, selector_str):
        _get_openeye_atom_predicate_base().__init__(self)
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
        _get_openeye_atom_predicate_base().__init__(copy)
        copy._cpp_selector = _CppOEResidueSelector(self._cpp_selector)
        return copy.__disown__()


class OEHasResidueName:
    """Match atoms by residue name with optional case/whitespace control.

    :param residue_name: The residue name to match.
    :param case_sensitive: If True, comparison is case-sensitive (default: False).
    :param whitespace: If True, whitespace is preserved (default: False).

    Example::

        pred = OEHasResidueName("ala")  # Matches "ALA", " ALA", etc.
    """

    _openeye_predicate_type = None

    def __new__(cls, *args, **kwargs):
        if cls is OEHasResidueName:
            predicate_cls = _get_lazy_openeye_predicate_type(cls)
            return predicate_cls.__new__(predicate_cls)
        return super().__new__(cls)

    def __init__(self, residue_name: str, case_sensitive: bool = False,
                 whitespace: bool = False):
        _get_openeye_atom_predicate_base().__init__(self)
        self._cpp_pred = _CppOEHasResidueName(residue_name, case_sensitive, whitespace)
        self._residue_name = residue_name
        self._case_sensitive = case_sensitive
        self._whitespace = whitespace

    def __call__(self, atom):
        return self._cpp_pred(atom)

    def CreateCopy(self):
        copy = OEHasResidueName(self._residue_name, self._case_sensitive, self._whitespace)
        return copy.__disown__()


class OEHasAtomNameAdvanced:
    """Match atoms by name with optional case/whitespace control.

    :param atom_name: The atom name to match.
    :param case_sensitive: If True, comparison is case-sensitive (default: False).
    :param whitespace: If True, whitespace is preserved (default: False).

    Example::

        pred = OEHasAtomNameAdvanced("ca")  # Matches "CA", " CA", etc.
    """

    _openeye_predicate_type = None

    def __new__(cls, *args, **kwargs):
        if cls is OEHasAtomNameAdvanced:
            predicate_cls = _get_lazy_openeye_predicate_type(cls)
            return predicate_cls.__new__(predicate_cls)
        return super().__new__(cls)

    def __init__(self, atom_name: str, case_sensitive: bool = False,
                 whitespace: bool = False):
        _get_openeye_atom_predicate_base().__init__(self)
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
    PredicateType_AND,
    PredicateType_OR,
    PredicateType_NOT,
    PredicateType_XOR,
    PredicateType_NAME,
    PredicateType_RESN,
    PredicateType_RESI,
    PredicateType_CHAIN,
    PredicateType_ELEM,
    PredicateType_INDEX,
    PredicateType_ID,
    PredicateType_ALT,
    PredicateType_B_FACTOR,
    PredicateType_FRAGMENT,
    PredicateType_SECONDARY_STRUCTURE,
    PredicateType_PROTEIN,
    PredicateType_LIGAND,
    PredicateType_WATER,
    PredicateType_SOLVENT,
    PredicateType_ORGANIC,
    PredicateType_BACKBONE,
    PredicateType_METAL,
    PredicateType_HEAVY,
    PredicateType_HYDROGEN,
    PredicateType_POLAR_HYDROGEN,
    PredicateType_NONPOLAR_HYDROGEN,
    PredicateType_BY_RES,
    PredicateType_BY_CHAIN,
    PredicateType_AROUND,
    PredicateType_EXPAND,
    PredicateType_BEYOND,
    PredicateType_HELIX,
    PredicateType_SHEET,
    PredicateType_TURN,
    PredicateType_LOOP,
    PredicateType_ALL_MATCH,
    PredicateType_NO_MATCH,
)

# Create a namespace for PredicateType enum
class PredicateType:
    """Enum-like class for predicate types."""
    And = PredicateType_AND
    Or = PredicateType_OR
    Not = PredicateType_NOT
    XOr = PredicateType_XOR
    Name = PredicateType_NAME
    Resn = PredicateType_RESN
    Resi = PredicateType_RESI
    Chain = PredicateType_CHAIN
    Elem = PredicateType_ELEM
    Index = PredicateType_INDEX
    Id = PredicateType_ID
    Alt = PredicateType_ALT
    BFactor = PredicateType_B_FACTOR
    Fragment = PredicateType_FRAGMENT
    SecondaryStructure = PredicateType_SECONDARY_STRUCTURE
    Protein = PredicateType_PROTEIN
    Ligand = PredicateType_LIGAND
    Water = PredicateType_WATER
    Solvent = PredicateType_SOLVENT
    Organic = PredicateType_ORGANIC
    Backbone = PredicateType_BACKBONE
    Metal = PredicateType_METAL
    Heavy = PredicateType_HEAVY
    Hydrogen = PredicateType_HYDROGEN
    PolarHydrogen = PredicateType_POLAR_HYDROGEN
    NonpolarHydrogen = PredicateType_NONPOLAR_HYDROGEN
    ByRes = PredicateType_BY_RES
    ByChain = PredicateType_BY_CHAIN
    Around = PredicateType_AROUND
    Expand = PredicateType_EXPAND
    Beyond = PredicateType_BEYOND
    Helix = PredicateType_HELIX
    Sheet = PredicateType_SHEET
    Turn = PredicateType_TURN
    Loop = PredicateType_LOOP
    True_ = PredicateType_ALL_MATCH
    False_ = PredicateType_NO_MATCH

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
    "parse_selector_set",
    "str_selector_set",
    "selector_set",
    "mol_to_selector_set",
    "get_selector_string",
]
