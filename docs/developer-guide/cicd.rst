CI/CD Pipeline
==============

This document describes the GitHub Actions CI/CD pipeline for building and distributing
OESelect Python wheels.

Overview
--------

The CI/CD pipeline automates:

- Building platform-specific Python wheels for Linux and macOS
- Repairing wheels for manylinux compliance (Linux) and bundling dependencies (macOS)
- Collecting all platform wheels into a single artifact
- Publishing releases to PyPI on version tags

Supported Platforms
-------------------

The pipeline builds wheels for the following targets:

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 20 20

   * - Platform
     - GitHub Runner
     - Container
     - Architecture
     - Wheel Tag
   * - RHEL 8
     - ``ubuntu-latest``
     - ``rockylinux:8``
     - x86_64
     - ``manylinux_2_28_x86_64``
   * - RHEL 9
     - ``ubuntu-24.04-arm``
     - ``rockylinux:9``
     - aarch64
     - ``manylinux_2_34_aarch64``
   * - macOS
     - ``macos-latest``
     - (native)
     - universal2
     - ``macosx_11_0_universal2``

All wheels are built with the Python stable ABI (abi3) targeting Python 3.10+, so a
single wheel per platform works across Python 3.10, 3.11, 3.12, and 3.13.

.. note::

   The aarch64 Linux build requires GitHub's ARM runners (``ubuntu-24.04-arm``),
   available on GitHub Team/Enterprise plans, public repositories, or via self-hosted
   runners.

Workflow Structure
------------------

The workflow file is located at ``.github/workflows/build-wheels.yml`` and consists of
four jobs:

**build-linux**
    Matrix job that builds wheels for x86_64 and aarch64 in parallel. Each job:

    1. Runs in a Rocky Linux container matching the target platform
    2. Installs build dependencies (compilers, CMake, Python, SWIG, cairo)
    3. Authenticates to Google Cloud via Workload Identity Federation
    4. Downloads the OpenEye C++ SDK from Google Cloud Storage
    5. Installs openeye-toolkits from the Anaconda PyPI channel
    6. Builds the wheel using ``scripts/build_python.py``
    7. Repairs the wheel with ``auditwheel`` for manylinux compliance
    8. Uploads the wheel as a GitHub artifact

**build-macos**
    Builds a universal2 (arm64 + x86_64) wheel on macOS. Uses ``delocate`` to bundle
    non-OpenEye dependencies and ``codesign`` to re-sign binaries.

**collect-wheels**
    Aggregates all platform wheels into a single downloadable artifact.

**publish**
    Publishes wheels to PyPI when a version tag is pushed (e.g., ``v1.0.0``).
    Uses PyPI trusted publishing for tokenless authentication.

Workflow Triggers
-----------------

The pipeline runs on:

- **Push to main/master**: Builds wheels for validation
- **Pull requests**: Builds wheels to verify changes don't break the build
- **Version tags**: Builds and publishes to PyPI (tags matching ``v*``)
- **Manual dispatch**: Can be triggered from the GitHub Actions UI

Required Secrets
----------------

The following secrets must be configured in the GitHub repository settings
(**Settings > Secrets and variables > Actions**):

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Secret Name
     - Description
   * - ``GCP_WORKLOAD_IDENTITY_PROVIDER``
     - Full resource name of the GCP Workload Identity Pool provider, e.g.
       ``projects/PROJECT_NUMBER/locations/global/workloadIdentityPools/POOL/providers/PROVIDER``
   * - ``GCP_SERVICE_ACCOUNT``
     - Service account email with ``roles/storage.objectViewer`` on the GCS bucket, e.g.
       ``ci-reader@project.iam.gserviceaccount.com``
   * - ``OE_LICENSE``
     - Full contents of the OpenEye license file (``oe_license.txt``)

SDK Storage
-----------

The OpenEye C++ SDK tarballs are stored in a Google Cloud Storage bucket. The bucket
name is configured via the ``GCS_BUCKET`` environment variable in the workflow (default:
``openeye-sdks``).

Expected SDK tarballs::

    gs://openeye-sdks/
    ├── OpenEye-toolkits-2025.2.1-redhat-RHEL8-g++8.5-x64.tar.gz
    ├── OpenEye-toolkits-2025.2.1-Ubuntu-22.04-g++11.11-aarch64.tar.gz
    └── OpenEye-toolkits-2025.2.1-macOS-14.8-clang++16-universal.tar.gz

Authentication uses GCP Workload Identity Federation, which allows GitHub Actions to
obtain short-lived access tokens without storing long-lived credentials.

Wheel Repair
------------

**Linux (auditwheel)**

After building, Linux wheels are repaired with ``auditwheel`` to ensure manylinux
compliance. OpenEye shared libraries are excluded from bundling since they are provided
at runtime by the ``openeye-toolkits`` Python package::

    OE_LIB_DIR=$(python -c "from openeye import libs; print(libs.FindOpenEyeDLLSDirectory())")
    for lib in "$OE_LIB_DIR"/lib*.so; do
      EXCLUDE_ARGS="$EXCLUDE_ARGS --exclude $(basename $lib)"
    done
    LD_LIBRARY_PATH="$OE_LIB_DIR" auditwheel repair $EXCLUDE_ARGS ...

**macOS (delocate)**

On macOS, ``delocate`` bundles non-OpenEye dylibs into the wheel. After delocating,
``install_name_tool`` adds an RPATH pointing to the openeye-toolkits library directory,
and ``codesign`` re-signs all binaries with an ad-hoc signature.

Local Development
-----------------

To build wheels locally, use the build script directly::

    python scripts/build_python.py --openeye-root /path/to/openeye/sdk --verbose

The script automatically discovers ``OPENEYE_ROOT`` from CMake presets if the flag is
omitted and the variable is not set in the environment.

See :doc:`../getting-started/installation` for detailed local build instructions.

Artifacts
---------

After a successful build, the following artifacts are available for download from the
GitHub Actions run:

- ``wheel-linux-x86_64``: x86_64 manylinux wheel
- ``wheel-linux-aarch64``: aarch64 manylinux wheel
- ``wheel-macos-universal2``: macOS universal wheel
- ``all-wheels``: Combined artifact containing all platform wheels

Artifacts are retained for 30 days.

Publishing to PyPI
------------------

The workflow uses PyPI's trusted publishing feature for secure, tokenless publishing.

To configure trusted publishing:

1. Create a GitHub environment named ``pypi`` (**Settings > Environments**)
2. On `pypi.org <https://pypi.org>`_, go to **Manage > Publishing** for the
   ``oeselect`` project and add a new trusted publisher:

   - **Owner**: Your GitHub username or organization
   - **Repository**: ``oeselect``
   - **Workflow**: ``build-wheels.yml``
   - **Environment**: ``pypi``

3. Create a version tag to trigger publishing::

       git tag v1.0.0
       git push origin v1.0.0

Troubleshooting
---------------

**Build fails with missing OpenEye headers**
    Verify the SDK tarball exists in GCS and the ``OPENEYE_ROOT`` path is correct.

**License validation fails**
    Ensure the ``OE_LICENSE`` secret contains the complete license file content.

**ARM64 build fails to start**
    ARM runners require GitHub Team/Enterprise plan, a public repository, or a
    self-hosted ARM runner.

**GCS download fails**
    Check that the service account has ``roles/storage.objectViewer`` on the GCS bucket,
    and that the Workload Identity Federation provider allows the repository.

**openeye import fails with libcairo error**
    The ``cairo`` package must be installed in the build container. OpenEye's ``oedepict``
    module requires ``libcairo.so.2``, which is loaded during toolkit initialization.
