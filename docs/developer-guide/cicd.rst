CI/CD Pipeline
==============

This document describes the GitHub Actions CI/CD pipeline for building and distributing
OESelect Python wheels across Linux platforms.

Overview
--------

The CI/CD pipeline automates:

- Building platform-specific Python wheels for multiple Linux distributions
- Running tests across all supported platforms
- Publishing releases to PyPI

Supported Platforms
-------------------

The pipeline builds wheels for the following Linux targets:

.. list-table::
   :header-rows: 1
   :widths: 25 20 20 15 20

   * - Platform
     - GitHub Runner
     - Container
     - Compiler
     - Architecture
   * - Ubuntu 22.04
     - ``ubuntu-latest``
     - ``ubuntu:22.04``
     - g++11
     - x64
   * - Ubuntu 22.04
     - ``ubuntu-24.04-arm``
     - ``ubuntu:22.04``
     - g++11
     - aarch64
   * - Ubuntu 24.04
     - ``ubuntu-latest``
     - ``ubuntu:24.04``
     - g++13
     - x64
   * - RHEL 8
     - ``ubuntu-latest``
     - ``rockylinux:8``
     - g++8.5
     - x64
   * - RHEL 9
     - ``ubuntu-latest``
     - ``rockylinux:9``
     - g++11
     - x64

.. note::

   ARM64 builds require GitHub's ARM runners (``ubuntu-24.04-arm``), which are available
   on GitHub Team/Enterprise plans or via self-hosted runners.

Workflow Structure
------------------

The workflow file is located at ``.github/workflows/build-wheels.yml`` and consists of
three jobs:

**build**
    Matrix job that builds wheels for each platform in parallel. Each job:

    1. Runs in a container matching the target platform
    2. Installs build dependencies (compilers, CMake, Python, SWIG)
    3. Downloads the OpenEye C++ SDK from cloud storage
    4. Builds the wheel using ``scripts/build_python.py``
    5. Uploads the wheel as a GitHub artifact

**collect-wheels**
    Aggregates all platform wheels into a single downloadable artifact.

**publish**
    Publishes wheels to PyPI when a version tag is pushed (e.g., ``v1.0.0``).

Workflow Triggers
-----------------

The pipeline runs on:

- **Push to main/master**: Builds wheels for validation
- **Pull requests**: Builds wheels to verify changes don't break the build
- **Version tags**: Builds and publishes to PyPI (tags matching ``v*``)
- **Manual dispatch**: Can be triggered manually from the GitHub Actions UI

Required Secrets
----------------

The following secrets must be configured in the GitHub repository settings:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Secret Name
     - Description
   * - ``AWS_ACCESS_KEY_ID``
     - AWS access key for downloading SDK from S3
   * - ``AWS_SECRET_ACCESS_KEY``
     - AWS secret key for downloading SDK from S3
   * - ``OE_LICENSE``
     - OpenEye license file content (required for building)

SDK Storage
-----------

The OpenEye C++ SDK tarballs are stored in an S3 bucket with the following structure::

    s3://<bucket-name>/openeye-sdk/
    ├── OpenEye-toolkits-<version>-Ubuntu-22.04-g++11.11-x64.tar.gz
    ├── OpenEye-toolkits-<version>-Ubuntu-22.04-g++11.11-aarch64.tar.gz
    ├── OpenEye-toolkits-<version>-Ubuntu-24.04-g++13.13-x64.tar.gz
    ├── OpenEye-toolkits-<version>-redhat-RHEL8-g++8.5-x64.tar.gz
    └── OpenEye-toolkits-<version>-redhat-RHEL9-g++11.11-x64.tar.gz

Configuration
-------------

Before using the workflow, update the following in ``.github/workflows/build-wheels.yml``:

1. **S3 bucket name**: Update the ``S3_BUCKET`` environment variable
2. **SDK filenames**: Update the ``sdk`` field in each matrix entry if using different SDK versions
3. **AWS region**: Update ``AWS_DEFAULT_REGION`` if your bucket is in a different region

Local Development
-----------------

To build wheels locally, use the build script directly::

    python scripts/build_python.py --openeye-root /path/to/openeye/sdk

See :doc:`../getting-started/installation` for detailed local build instructions.

Artifacts
---------

After a successful build, the following artifacts are available for download from the
GitHub Actions run:

- ``wheel-<platform>``: Individual wheel for each platform
- ``all-linux-wheels``: Combined artifact containing all platform wheels

Artifacts are retained for 30 days by default.

Publishing to PyPI
------------------

The workflow uses PyPI's trusted publishing feature for secure, tokenless publishing.
To enable this:

1. Go to your PyPI project settings
2. Add a new trusted publisher with:

   - Owner: Your GitHub username or organization
   - Repository: The repository name
   - Workflow: ``build-wheels.yml``
   - Environment: ``pypi``

3. Create a version tag to trigger publishing::

       git tag v1.0.0
       git push origin v1.0.0

Troubleshooting
---------------

**Build fails with missing OpenEye headers**
    Verify the SDK tarball exists in S3 and the ``OPENEYE_ROOT`` path is correct.

**License validation fails**
    Ensure the ``OE_LICENSE`` secret contains the complete license file content.

**ARM64 build fails to start**
    ARM runners require GitHub Team/Enterprise plan or a self-hosted ARM runner.

**S3 download fails**
    Check that AWS credentials have ``s3:GetObject`` permission on the SDK bucket.
