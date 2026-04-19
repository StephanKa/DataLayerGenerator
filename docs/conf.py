"""Sphinx configuration for the DataLayerGenerator documentation."""
# Configuration file for the Sphinx documentation builder.
#
# Install build dependencies:
#   pip install sphinx sphinx-rtd-theme breathe sphinx-copybutton sphinxcontrib-plantuml
#
# Build:
#   sphinx-build -b html docs/ build/docs/sphinx/

import os

# -- Project information -----------------------------------------------------
project = 'DataLayerGenerator'
copyright = '2025, Kantelberg Stephan'
author = 'Kantelberg Stephan'
release = '0.0.1'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',
    'sphinx_copybutton',
]

# sphinxcontrib.plantuml is optional — only enable when plantuml is available
if os.environ.get('ENABLE_PLANTUML', '0') == '1':
    extensions.append('sphinxcontrib.plantuml')

# The master toctree document.
master_doc = 'index'

source_suffix = '.rst'

# Exclude build output from source search
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Breathe (Doxygen XML bridge) --------------------------------------------
breathe_default_project = 'DataLayerGenerator'
# breathe_projects is injected at build time via:
#   -Dbreathe_projects.DataLayerGenerator=<doxygen_xml_dir>

# -- HTML output -------------------------------------------------------------
html_theme = 'sphinx_rtd_theme'

html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'titles_only': False,
}

html_static_path = []

# -- Copy-button configuration -----------------------------------------------
copybutton_prompt_text = r'>>> |\.\.\. |\$ '
copybutton_prompt_is_regexp = True
