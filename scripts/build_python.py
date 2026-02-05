#!/usr/bin/env python3
"""
Build and package oeselect for PyPI distribution.

This script builds:
1. oeselect-lib: Binary wheel with compiled extensions
2. oeselect: Source distribution (sdist) metapackage

All packages are placed in dist/ ready for upload to PyPI.

Usage:
    python scripts/build_python.py [options]

Options:
    --openeye-root PATH    Path to OpenEye C++ SDK (headers)
    --python PATH          Python executable to use
    --skip-lib             Skip building oeselect-lib wheel
    --skip-meta            Skip building oeselect metapackage
    --clean                Clean dist/ before building
    --upload               Upload to PyPI after building (requires twine)
    --test-upload          Upload to TestPyPI instead of PyPI
    --verbose              Verbose output

Environment Variables:
    OPENEYE_ROOT    Path to OpenEye C++ SDK (alternative to --openeye-root)
    PYTHON          Python executable (alternative to --python)

Examples:
    # Build everything
    python scripts/build_python.py --openeye-root /path/to/openeye/sdk

    # Build and upload to TestPyPI
    python scripts/build_python.py --openeye-root /path/to/openeye/sdk --upload --test-upload

    # Build only the metapackage
    python scripts/build_python.py --skip-lib
"""

import argparse
import io
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

    @classmethod
    def disable(cls):
        cls.RED = cls.GREEN = cls.YELLOW = cls.BLUE = cls.NC = ''


def print_header(msg):
    print(f"\n{Colors.GREEN}{'=' * 60}{Colors.NC}")
    print(f"{Colors.GREEN}{msg}{Colors.NC}")
    print(f"{Colors.GREEN}{'=' * 60}{Colors.NC}\n")


def print_step(msg):
    print(f"{Colors.YELLOW}>>> {msg}{Colors.NC}")


def print_error(msg):
    print(f"{Colors.RED}ERROR: {msg}{Colors.NC}", file=sys.stderr)


def print_success(msg):
    print(f"{Colors.GREEN}{msg}{Colors.NC}")


def run_command(cmd, cwd=None, check=True, capture_output=False, verbose=False):
    """Run a command and optionally capture output."""
    if verbose:
        print(f"{Colors.BLUE}Running: {' '.join(cmd)}{Colors.NC}")

    result = subprocess.run(
        cmd,
        cwd=cwd,
        check=check,
        capture_output=capture_output,
        text=True
    )
    return result


def get_openeye_info(python_exe):
    """Get OpenEye toolkits version and library directory from Python."""
    code = """
from openeye import libs, oechem
import os
dll_dir = libs.FindOpenEyeDLLSDirectory()
version = oechem.OEToolkitsGetRelease()
print(f'VERSION:{version}')
print(f'LIB_DIR:{dll_dir}')
print(f'PLATFORM:{os.path.basename(dll_dir)}')
"""
    try:
        result = run_command(
            [python_exe, '-c', code],
            capture_output=True,
            check=True
        )
        info = {}
        for line in result.stdout.strip().split('\n'):
            if ':' in line:
                key, value = line.split(':', 1)
                info[key] = value
        return info
    except subprocess.CalledProcessError:
        return None


def verify_openeye_root(openeye_root):
    """Verify OpenEye C++ SDK path has headers."""
    include_dir = Path(openeye_root) / 'include'
    if not include_dir.exists():
        print_error(f"OpenEye include directory not found: {include_dir}")
        return False
    if not (include_dir / 'oechem.h').exists():
        print_error(f"oechem.h not found in {include_dir}")
        return False
    return True


def get_version_from_pyproject(pyproject_path):
    """Extract version string from a pyproject.toml file."""
    if not pyproject_path.exists():
        return None
    content = pyproject_path.read_text()
    match = re.search(r'version\s*=\s*"([^"]+)"', content)
    if match:
        return match.group(1)
    return None


def verify_package_versions(project_dir):
    """Verify that oeselect and oeselect-lib have the same version."""
    lib_pyproject = Path(project_dir) / 'pyproject.toml'
    meta_pyproject = Path(project_dir) / 'metapackage' / 'pyproject.toml'

    lib_version = get_version_from_pyproject(lib_pyproject)
    meta_version = get_version_from_pyproject(meta_pyproject)

    if lib_version is None:
        print_error(f"Could not extract version from {lib_pyproject}")
        return False

    if meta_version is None:
        print_error(f"Could not extract version from {meta_pyproject}")
        return False

    if lib_version != meta_version:
        print_error(f"Version mismatch: oeselect-lib={lib_version}, oeselect={meta_version}")
        print_error("Please ensure both packages have the same version in their pyproject.toml files")
        return False

    print_step(f"oeselect and oeselect-lib versions match: {lib_version}")
    return True


def build_oeselect_lib(project_dir, python_exe, openeye_root, openeye_info, verbose=False):
    """Build the oeselect-lib binary wheel."""
    print_header("Building oeselect-lib (binary wheel)")

    openeye_version = openeye_info['VERSION']
    openeye_lib_dir = openeye_info['LIB_DIR']

    print_step(f"OpenEye Toolkits version: {openeye_version}")
    print_step(f"OpenEye library directory: {openeye_lib_dir}")
    print_step(f"OpenEye C++ SDK: {openeye_root}")

    # Build the wheel
    print_step("Building wheel with pip...")
    cmd = [
        python_exe, '-m', 'pip', 'wheel', '.',
        '--no-build-isolation',
        '--no-deps',
        '--wheel-dir', 'dist',
        f'-C', f'cmake.define.OPENEYE_ROOT={openeye_root}',
        f'-C', f'cmake.define.OPENEYE_LIB_DIR={openeye_lib_dir}',
        f'-C', f'cmake.define.OPENEYE_TOOLKITS_VERSION={openeye_version}',
        '-C', 'cmake.define.OPENEYE_USE_SHARED=ON',
        '-C', 'cmake.define.OESELECT_BUILD_TESTS=OFF',
        '-C', 'logging.level=INFO',
    ]
    run_command(cmd, cwd=project_dir, verbose=verbose)

    # Find the built wheel
    wheels = list(Path(project_dir, 'dist').glob('oeselect_lib-*.whl'))
    if not wheels:
        print_error("No wheel file created")
        return None

    wheel_file = wheels[0]
    print_step(f"Wheel built: {wheel_file.name}")

    # Rename wheel to include OpenEye version
    wheel_name = wheel_file.name
    match = re.match(r'^([^-]+)-([^-]+)-(.+)$', wheel_name)
    if match:
        pkg_name, pkg_version, rest = match.groups()
        if '+oe' not in pkg_version:
            new_name = f"{pkg_name}-{pkg_version}+oe{openeye_version}-{rest}"
            new_path = wheel_file.parent / new_name
            wheel_file.rename(new_path)
            wheel_file = new_path
            print_step(f"Renamed to: {wheel_file.name}")

    # Run delocate if available (macOS)
    if platform.system() == 'Darwin':
        wheel_file = run_delocate(project_dir, python_exe, wheel_file, openeye_info, verbose)

    print_success(f"oeselect-lib wheel: {wheel_file}")
    return wheel_file


def run_delocate(project_dir, python_exe, wheel_file, openeye_info, verbose=False):
    """Run delocate to bundle non-OpenEye dependencies (macOS only)."""
    try:
        run_command([python_exe, '-c', 'import delocate'], capture_output=True, check=True)
    except subprocess.CalledProcessError:
        print_step("delocate not installed, skipping dependency bundling")
        return wheel_file

    print_step("Running delocate to bundle dependencies...")

    delocated_dir = Path(project_dir) / 'dist' / 'delocated'
    delocated_dir.mkdir(exist_ok=True)

    # Libraries we expect to be missing (provided at runtime by other packages)
    # - OpenEye libraries: provided by openeye-toolkits
    expected_missing = {'liboechem', 'liboesystem', 'liboeplatform', 'liboegraphsim',
                        'liboemath', 'libzstd', 'liboeshape', 'liboeff', 'liboegrid',
                        'liboespicoli', 'liboezap', 'liboequacpac', 'liboedepict',
                        'liboegrapheme', 'liboemolprop', 'liboeiupac', 'liboefastrocs'}

    try:
        # Run delocate and capture output to filter expected missing library warnings
        result = subprocess.run([
            python_exe, '-m', 'delocate.cmd.delocate_wheel',
            '-w', str(delocated_dir),
            '-v',
            '--ignore-missing-dependencies',
            str(wheel_file)
        ], capture_output=True, text=True)

        # Filter and display output - hide expected OpenEye library warnings
        if result.stdout and verbose:
            print(result.stdout)

        if result.stderr:
            # Filter out expected missing library errors
            # Delocate outputs multi-line messages, so we need to look at blocks
            lines = result.stderr.split('\n')
            i = 0
            while i < len(lines):
                line = lines[i].strip()
                i += 1
                if not line:
                    continue

                # Check if this line or nearby lines contain expected missing libraries
                # Look ahead a few lines for context
                context = ' '.join(lines[max(0, i-1):min(len(lines), i+5)])
                is_expected = any(lib in context for lib in expected_missing)

                if is_expected:
                    # Skip this and any continuation lines until next message type
                    while i < len(lines) and not lines[i].strip().startswith(('INFO:', 'ERROR:', 'WARNING:')):
                        i += 1
                    continue

                # Show INFO messages about what delocate is doing
                if line.startswith('INFO:delocate'):
                    # Show useful info like copying libraries
                    if 'Copying' in line or 'Modifying' in line or 'Output:' in line:
                        print(f"  {line}")
                elif line.startswith('ERROR:') or line.startswith('WARNING:'):
                    # Show unexpected errors (shouldn't happen if filtering works)
                    print(f"  {line}")

        # Check if delocate succeeded (return code 0 is success)
        if result.returncode != 0:
            print_step(f"delocate returned non-zero exit code: {result.returncode}")
            # Still continue - the wheel may be usable

        # Use delocated wheel if it exists
        delocated_wheels = list(delocated_dir.glob('*.whl'))
        if delocated_wheels:
            # Remove original
            wheel_file.unlink()
            # Move delocated wheel
            new_wheel = delocated_wheels[0]
            final_path = wheel_file.parent / new_wheel.name
            shutil.move(str(new_wheel), str(final_path))
            wheel_file = final_path

            # Add RPATH back for OpenEye libraries
            print_step("Adding RPATH for openeye-toolkits libraries...")
            wheel_file = fix_rpath_and_sign(wheel_file, openeye_info)

    except Exception as e:
        print_step(f"delocate warning: {e}")

    # Clean up delocated directory
    if delocated_dir.exists():
        shutil.rmtree(delocated_dir, ignore_errors=True)

    return wheel_file


def fix_rpath_and_sign(wheel_file, openeye_info):
    """Fix RPATH and re-sign binary after delocate (macOS)."""
    import zipfile

    openeye_platform = openeye_info['PLATFORM']

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # Extract wheel
        with zipfile.ZipFile(wheel_file, 'r') as zf:
            zf.extractall(tmpdir)

        # Find the .so file
        so_file = tmpdir / 'oeselect_lib' / '_oeselect.so'
        if so_file.exists():
            # Add rpath
            try:
                subprocess.run([
                    'install_name_tool', '-add_rpath',
                    f'@loader_path/../openeye/libs/{openeye_platform}',
                    str(so_file)
                ], check=False, capture_output=True)

                # Re-sign
                subprocess.run([
                    'codesign', '-f', '-s', '-', str(so_file)
                ], check=False, capture_output=True)

                # Sign bundled dylibs too
                dylibs_dir = tmpdir / 'oeselect_lib' / '.dylibs'
                if dylibs_dir.exists():
                    for dylib in dylibs_dir.glob('*.dylib'):
                        subprocess.run([
                            'codesign', '-f', '-s', '-', str(dylib)
                        ], check=False, capture_output=True)

                print_step("RPATH added and binary re-signed")
            except Exception as e:
                print_step(f"Warning: Could not fix RPATH: {e}")

        # Repackage wheel
        wheel_file.unlink()
        with zipfile.ZipFile(wheel_file, 'w', zipfile.ZIP_DEFLATED) as zf:
            for file_path in tmpdir.rglob('*'):
                if file_path.is_file():
                    arcname = file_path.relative_to(tmpdir)
                    zf.write(file_path, arcname)

    return wheel_file


def build_oeselect_meta(project_dir, python_exe, verbose=False):
    """Build the oeselect metapackage source distribution."""
    print_header("Building oeselect (metapackage sdist)")

    metapackage_dir = Path(project_dir) / 'metapackage'
    if not metapackage_dir.exists():
        print_error(f"Metapackage directory not found: {metapackage_dir}")
        return None

    # Build sdist using build module or setup.py
    print_step("Building source distribution...")

    sdist_file = None

    # Try using python -m build first
    try:
        run_command(
            [python_exe, '-m', 'build', '--sdist', '--no-isolation',
             '--outdir', str(Path(project_dir) / 'dist')],
            cwd=metapackage_dir,
            verbose=verbose
        )
        sdists = list(Path(project_dir, 'dist').glob('oeselect-*.tar.gz'))
        if sdists:
            sdist_file = sdists[0]
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        if verbose:
            print_step(f"python -m build failed: {e}")

    # Fall back to setup.py sdist
    if not sdist_file:
        try:
            print_step("Trying setup.py sdist...")
            run_command(
                [python_exe, 'setup.py', 'sdist', '--dist-dir', str(Path(project_dir) / 'dist')],
                cwd=metapackage_dir,
                verbose=verbose
            )
            sdists = list(Path(project_dir, 'dist').glob('oeselect-*.tar.gz'))
            if sdists:
                sdist_file = sdists[0]
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            if verbose:
                print_step(f"setup.py sdist failed: {e}")

    # Final fallback: create sdist manually
    if not sdist_file:
        print_step("Creating sdist manually...")
        sdist_file = create_sdist_manual(metapackage_dir, Path(project_dir) / 'dist')

    if not sdist_file or not sdist_file.exists():
        print_error("No sdist file created")
        return None

    print_success(f"oeselect sdist: {sdist_file}")
    return sdist_file


def create_sdist_manual(metapackage_dir, dist_dir):
    """Create source distribution manually without setuptools."""
    metapackage_dir = Path(metapackage_dir)
    dist_dir = Path(dist_dir)
    dist_dir.mkdir(exist_ok=True)

    # Get package info from pyproject.toml or setup.py
    pkg_name = "oeselect"
    pkg_version = "1.0.0"

    # Try to extract version from pyproject.toml
    pyproject_path = metapackage_dir / 'pyproject.toml'
    if pyproject_path.exists():
        content = pyproject_path.read_text()
        match = re.search(r'version\s*=\s*"([^"]+)"', content)
        if match:
            pkg_version = match.group(1)

    sdist_name = f"{pkg_name}-{pkg_version}"
    sdist_filename = f"{sdist_name}.tar.gz"
    sdist_path = dist_dir / sdist_filename

    # Files to include in sdist
    files_to_include = [
        'pyproject.toml',
        'setup.py',
        'README.md',
        'LICENSE',
    ]

    # Include src/ directory
    src_dir = metapackage_dir / 'src'

    with tarfile.open(sdist_path, 'w:gz') as tar:
        # Add top-level files
        for filename in files_to_include:
            filepath = metapackage_dir / filename
            if filepath.exists():
                arcname = f"{sdist_name}/{filename}"
                tar.add(filepath, arcname=arcname)

        # Add src/ directory
        if src_dir.exists():
            for filepath in src_dir.rglob('*'):
                if filepath.is_file() and '__pycache__' not in str(filepath):
                    arcname = f"{sdist_name}/src/{filepath.relative_to(src_dir)}"
                    tar.add(filepath, arcname=arcname)

        # Create PKG-INFO
        pkg_info = f"""Metadata-Version: 2.1
Name: {pkg_name}
Version: {pkg_version}
Summary: PyMOL-style selection language for OpenEye Toolkits
Requires-Python: >=3.10
"""
        pkg_info_bytes = pkg_info.encode('utf-8')
        tarinfo = tarfile.TarInfo(name=f"{sdist_name}/PKG-INFO")
        tarinfo.size = len(pkg_info_bytes)
        tar.addfile(tarinfo, io.BytesIO(pkg_info_bytes))

    return sdist_path


def upload_to_pypi(dist_dir, test_pypi=False, verbose=False):
    """Upload packages to PyPI using twine."""
    print_header(f"Uploading to {'TestPyPI' if test_pypi else 'PyPI'}")

    packages = list(Path(dist_dir).glob('oeselect*.whl')) + list(Path(dist_dir).glob('oeselect*.tar.gz'))
    if not packages:
        print_error("No packages found to upload")
        return False

    print_step(f"Packages to upload:")
    for pkg in packages:
        print(f"  - {pkg.name}")

    cmd = ['twine', 'upload']
    if test_pypi:
        cmd.extend(['--repository', 'testpypi'])
    cmd.extend([str(p) for p in packages])

    try:
        run_command(cmd, verbose=verbose)
        print_success("Upload complete!")
        return True
    except subprocess.CalledProcessError as e:
        print_error(f"Upload failed: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Build and package oeselect for PyPI distribution",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        '--openeye-root',
        default=os.environ.get('OPENEYE_ROOT') or os.environ.get('OE_DIR'),
        help='Path to OpenEye C++ SDK (headers)'
    )
    parser.add_argument(
        '--python',
        default=os.environ.get('PYTHON', sys.executable),
        help='Python executable to use'
    )
    parser.add_argument(
        '--skip-lib',
        action='store_true',
        help='Skip building oeselect-lib wheel'
    )
    parser.add_argument(
        '--skip-meta',
        action='store_true',
        help='Skip building oeselect metapackage'
    )
    parser.add_argument(
        '--clean',
        action='store_true',
        help='Clean dist/ before building'
    )
    parser.add_argument(
        '--upload',
        action='store_true',
        help='Upload to PyPI after building'
    )
    parser.add_argument(
        '--test-upload',
        action='store_true',
        help='Upload to TestPyPI instead of PyPI'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )
    parser.add_argument(
        '--no-color',
        action='store_true',
        help='Disable colored output'
    )

    args = parser.parse_args()

    if args.no_color:
        Colors.disable()

    # Get project directory
    script_dir = Path(__file__).parent.resolve()
    project_dir = script_dir.parent

    print_header("OESelect Python Package Builder")
    print(f"Project directory: {project_dir}")
    print(f"Python: {args.python}")

    # Verify package versions match
    if not verify_package_versions(project_dir):
        return 1

    # Clean dist if requested
    dist_dir = project_dir / 'dist'
    if args.clean and dist_dir.exists():
        print_step("Cleaning dist/...")
        for f in dist_dir.glob('oeselect*'):
            f.unlink()

    dist_dir.mkdir(exist_ok=True)

    # Get OpenEye info
    openeye_info = get_openeye_info(args.python)
    if openeye_info:
        print(f"OpenEye Toolkits: {openeye_info['VERSION']}")
    else:
        print_step("openeye-toolkits not installed in Python environment")
        if not args.skip_lib:
            print_error("Cannot build oeselect-lib without openeye-toolkits")
            print("Install with: pip install openeye-toolkits")
            print("Or use --skip-lib to only build the metapackage")
            return 1

    built_packages = []

    # Build oeselect-lib
    if not args.skip_lib:
        if not args.openeye_root:
            print_error("OPENEYE_ROOT not set")
            print("Please set OPENEYE_ROOT environment variable or use --openeye-root")
            return 1

        if not verify_openeye_root(args.openeye_root):
            return 1

        wheel = build_oeselect_lib(
            project_dir,
            args.python,
            args.openeye_root,
            openeye_info,
            verbose=args.verbose
        )
        if wheel:
            built_packages.append(wheel)
        else:
            print_error("Failed to build oeselect-lib")
            return 1

    # Build oeselect metapackage
    if not args.skip_meta:
        sdist = build_oeselect_meta(project_dir, args.python, verbose=args.verbose)
        if sdist:
            built_packages.append(sdist)
        else:
            print_error("Failed to build oeselect metapackage")
            return 1

    # Summary
    print_header("Build Summary")
    print("Built packages:")
    for pkg in built_packages:
        size_kb = pkg.stat().st_size / 1024
        print(f"  {pkg.name} ({size_kb:.1f} KB)")

    print(f"\nPackages are in: {dist_dir}")

    # Upload if requested
    if args.upload:
        if not upload_to_pypi(dist_dir, test_pypi=args.test_upload, verbose=args.verbose):
            return 1

    print_header("Done!")
    print("To upload to PyPI:")
    print(f"  twine upload {dist_dir}/oeselect*")
    print("\nTo upload to TestPyPI:")
    print(f"  twine upload --repository testpypi {dist_dir}/oeselect*")

    return 0


if __name__ == '__main__':
    sys.exit(main())
