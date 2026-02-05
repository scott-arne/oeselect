"""
Invoke tasks for OESelect project management
"""

import os
import sys
from pathlib import Path

from invoke import task

# Project paths
PROJECT_ROOT = Path(__file__).parent.absolute()
DOCS_DIR = PROJECT_ROOT / "docs"
BUILD_DIR = DOCS_DIR / "_build"
HTML_DIR = BUILD_DIR / "html"


@task
def docs(ctx, clean=False):
    """Build Sphinx documentation.

    :param clean: Remove build directory first.
    """
    os.chdir(DOCS_DIR)

    if clean:
        print("Cleaning build directory...")
        ctx.run("make clean", warn=True)

    print("Building documentation...")
    result = ctx.run("make html", warn=True)

    if result.ok:
        print(f"\nDocumentation built successfully!")
        print(f"Open: file://{HTML_DIR}/index.html")
    else:
        print("\nDocumentation build failed.", file=sys.stderr)
        sys.exit(1)


@task
def serve_docs(ctx, port=8000, clean=False):
    """Build documentation and serve at localhost.

    :param port: Port to serve on (default: 8000).
    :param clean: Clean build first.
    """
    # Build first
    docs(ctx, clean=clean)

    # Serve
    print(f"\nServing documentation at http://localhost:{port}")
    print("Press Ctrl+C to stop.\n")

    os.chdir(HTML_DIR)
    ctx.run(f"{sys.executable} -m http.server {port}")


@task
def docs_check(ctx):
    """Build documentation with warnings as errors (for CI)."""
    os.chdir(DOCS_DIR)

    print("Building documentation with strict checking...")
    result = ctx.run("make check", warn=True)

    if result.ok:
        print("\nDocumentation check passed!")
    else:
        print("\nDocumentation check failed - fix warnings.", file=sys.stderr)
        sys.exit(1)


@task
def docs_deps(ctx):
    """Install documentation dependencies."""
    print("Installing documentation dependencies...")
    ctx.run(f"{sys.executable} -m pip install -r {DOCS_DIR}/requirements.txt")
    print("Done!")
