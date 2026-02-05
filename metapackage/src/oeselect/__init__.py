"""
oeselect - PyMOL-style selection language for OpenEye Toolkits

This is the main oeselect package that users should install and import.
The actual implementation is in oeselect-lib, which is automatically
installed to match your openeye-toolkits version.

Usage::

    >>> from oeselect import select, count, parse
    >>>
    >>> # With an OpenEye molecule
    >>> from openeye import oechem
    >>> mol = oechem.OEGraphMol()
    >>> oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin
    >>>
    >>> # Select atoms by element
    >>> carbon_indices = select("elem C", mol)
    >>>
    >>> # Count atoms matching selection
    >>> num_oxygens = count("elem O", mol)
    >>>
    >>> # Parse and validate selection string
    >>> sele = parse("protein and chain A")
    >>> print(sele.ToCanonical())
"""

__version__ = "1.0.0"

import sys
import platform


def _get_openeye_version():
    """Get the installed OpenEye Toolkits version."""
    try:
        from openeye import oechem
        return oechem.OEToolkitsGetRelease()
    except ImportError:
        return None


def _get_oeselect_lib_version():
    """Get the installed oeselect-lib version and its OpenEye build version."""
    try:
        import oeselect_lib
        pkg_version = getattr(oeselect_lib, '__version__', 'unknown')

        # Try to get the OpenEye version it was built against
        try:
            from oeselect_lib import _build_info
            oe_build_version = getattr(_build_info, 'OPENEYE_BUILD_VERSION', None)
        except ImportError:
            oe_build_version = None

        return pkg_version, oe_build_version
    except ImportError:
        return None, None


def _get_python_tag():
    """Get the Python version tag for wheel naming."""
    v = sys.version_info
    return f"cp{v.major}{v.minor}"


def _get_platform_tag():
    """Get a simplified platform description."""
    system = platform.system()
    machine = platform.machine()
    return f"{system} {machine}"


def _check_and_import():
    """Check for oeselect-lib and import if compatible."""
    oe_version = _get_openeye_version()
    lib_version, lib_oe_version = _get_oeselect_lib_version()

    # Case 1: OpenEye not installed
    if oe_version is None:
        raise ImportError(
            "openeye-toolkits is not installed.\n"
            "Please install it first:\n"
            "  pip install openeye-toolkits\n"
            "\n"
            "Then reinstall oeselect:\n"
            "  pip install --force-reinstall oeselect"
        )

    # Case 2: oeselect-lib not installed
    if lib_version is None:
        raise ImportError(
            f"oeselect-lib binary package is not installed.\n"
            f"\n"
            f"Your environment:\n"
            f"  - openeye-toolkits: {oe_version}\n"
            f"  - Python: {_get_python_tag()}\n"
            f"  - Platform: {_get_platform_tag()}\n"
            f"\n"
            f"To install the compatible binary, run:\n"
            f"  pip install oeselect-lib==1.0.0+oe{oe_version}\n"
            f"\n"
            f"If no pre-built wheel is available, build from source:\n"
            f"  git clone https://github.com/yourusername/oeselect\n"
            f"  cd oeselect\n"
            f"  python scripts/build_python.py --openeye-root /path/to/openeye/sdk"
        )

    # Case 3: Version mismatch
    if lib_oe_version and lib_oe_version != oe_version:
        raise ImportError(
            f"oeselect-lib version mismatch.\n"
            f"\n"
            f"Installed oeselect-lib was built for OpenEye Toolkits {lib_oe_version}\n"
            f"but you have OpenEye Toolkits {oe_version}\n"
            f"\n"
            f"To fix, reinstall the compatible binary:\n"
            f"  pip uninstall oeselect-lib\n"
            f"  pip install oeselect-lib==1.0.0+oe{oe_version}"
        )

    # All checks passed, import oeselect_lib
    import oeselect_lib
    return oeselect_lib


# Try to import and re-export oeselect_lib
try:
    _lib = _check_and_import()

    # Re-export everything from oeselect_lib
    from oeselect_lib import *
    from oeselect_lib import __all__ as _lib_all

    # Update __all__ to include our additions
    __all__ = list(_lib_all) + ["__version__"]

except ImportError as e:
    # Store the error to show when user tries to use the module
    _import_error = e
    __all__ = ["__version__"]

    # Create stub functions that raise the import error
    def __getattr__(name):
        if name.startswith('_'):
            raise AttributeError(f"module 'oeselect' has no attribute '{name}'")
        raise _import_error
