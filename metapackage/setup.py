"""
oeselect metapackage - dynamically determines oeselect-lib dependency.

This setup.py computes the correct oeselect-lib version based on the
installed openeye-toolkits version at install time.
"""

from setuptools import setup


def get_openeye_version():
    """Detect the installed OpenEye Toolkits version."""
    try:
        from openeye import oechem
        return oechem.OEToolkitsGetRelease()
    except ImportError:
        return None


def get_install_requires():
    """
    Compute install_requires dynamically based on environment.

    Returns the correct oeselect-lib version to match the installed
    openeye-toolkits version.
    """
    oe_version = get_openeye_version()

    install_requires = [
        "openeye-toolkits",
    ]

    if oe_version:
        # Pin to the exact oeselect-lib version matching the OpenEye version
        oeselect_lib_req = f"oeselect-lib==1.0.0+oe{oe_version}"
        print(f"Detected openeye-toolkits {oe_version}")
        print(f"Adding dependency: {oeselect_lib_req}")
        install_requires.append(oeselect_lib_req)
    else:
        # OpenEye not installed yet - will be installed as dependency
        # User will need to run oeselect-setup after installation
        print("openeye-toolkits not yet installed")
        print("After installation, run 'oeselect-setup' to install the binary")

    return install_requires


setup(
    install_requires=get_install_requires(),
)
