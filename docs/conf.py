# OESelect Documentation Configuration
# Sphinx configuration for unified C++ and Python documentation

import os
import shutil
import sys

# -- Path Setup ---------------------------------------------------------------

# Add Python packages to path for autodoc
docs_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(docs_dir)

# Add oeselect_lib (Python bindings)
sys.path.insert(0, os.path.join(project_root, 'python', 'oeselect_lib'))

# Add metapackage
sys.path.insert(0, os.path.join(project_root, 'metapackage', 'src'))

# -- Check for Doxygen --------------------------------------------------------

# Doxygen is required for C++ documentation. If not available, we'll skip
# the C++ API section and only build Python documentation.
DOXYGEN_AVAILABLE = shutil.which('doxygen') is not None

if not DOXYGEN_AVAILABLE:
    print("WARNING: Doxygen not found. C++ API documentation will be skipped.")
    print("         Install doxygen to enable C++ documentation: brew install doxygen")

# -- Project Information ------------------------------------------------------

project = 'OESelect'
copyright = '2024-2026, OESelect Contributors'
author = 'OESelect Contributors'
release = '1.0.0'
version = '1.0'

# -- General Configuration ----------------------------------------------------

extensions = [
    # Python Documentation
    'sphinx.ext.autodoc',          # Auto-extract Python docstrings
    'sphinx.ext.napoleon',         # Google/NumPy docstring support
    'sphinx_autodoc_typehints',    # Type hint rendering

    # Cross-references and navigation
    'sphinx.ext.intersphinx',      # Cross-project links
    'sphinx.ext.viewcode',         # Link to source code
    'sphinx.ext.autosummary',      # Generate summary tables
]

# C++ Documentation (requires Doxygen)
if DOXYGEN_AVAILABLE:
    extensions.insert(0, 'breathe')   # Doxygen XML to Sphinx bridge
    extensions.insert(1, 'exhale')    # Auto-generate C++ API tree

templates_path = ['_templates']
exclude_patterns = ['_build', '_doxygen', 'Thumbs.db', '.DS_Store']

# -- Breathe Configuration (Doxygen -> Sphinx) --------------------------------

if DOXYGEN_AVAILABLE:
    breathe_projects = {'oeselect': '_doxygen/xml'}
    breathe_default_project = 'oeselect'
    breathe_default_members = ('members', 'undoc-members')

    # -- Exhale Configuration (Auto-generate C++ API tree) --------------------

    exhale_args = {
        'containmentFolder': './cpp-api',
        'rootFileName': 'library_root.rst',
        'rootFileTitle': 'C++ API Reference',
        'doxygenStripFromPath': '../include',
        'createTreeView': True,
        'exhaleExecutesDoxygen': True,
        'exhaleDoxygenStdin': '''INPUT = ../include/oeselect
RECURSIVE = YES
EXTRACT_ALL = YES
GENERATE_XML = YES
XML_OUTPUT = xml
GENERATE_HTML = NO
GENERATE_LATEX = NO
QUIET = YES
JAVADOC_AUTOBRIEF = YES
QT_AUTOBRIEF = YES''',
    }

# -- Napoleon Configuration (Docstring parsing) -------------------------------

napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = True
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = True
napoleon_use_admonition_for_notes = True
napoleon_use_admonition_for_references = True
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_type_aliases = None

# -- Autodoc Configuration ----------------------------------------------------

autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
    'special-members': '__init__',
    'undoc-members': True,
    'exclude-members': '__weakref__',
}

autodoc_typehints = 'description'
autodoc_typehints_format = 'short'

# -- Intersphinx Configuration ------------------------------------------------

# Intersphinx links to Python docs (may fail behind firewall/proxy)
intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
}
intersphinx_timeout = 10  # Fail fast if network unreachable

# -- HTML Output Configuration ------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

html_theme_options = {
    'logo_only': False,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False,
}

# -- Custom CSS (create _static/custom.css if needed) -------------------------

# def setup(app):
#     app.add_css_file('custom.css')
