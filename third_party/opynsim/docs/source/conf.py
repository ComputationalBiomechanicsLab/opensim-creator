# -- Project information -----------------------------------------------------

project = 'OPynSim'
copyright = '2026, Adam Kewley'
author = 'Adam Kewley'
github_username = 'adamkewley'
github_repository = 'https://github.com/opynsim/opynsim'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
]

# Napoleon settings
napoleon_google_docstring = True
napoleon_numpy_docstring = False  # `opynsim` uses Google-style docstrings

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# enable numbering each figure (e.g. "figure 1", "figure 2" in caption)
numfig = True


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_book_theme'
html_logo = '_static/opynsim_banner_vertical.svg'
html_favicon = '_static/opynsim_logo.svg'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']
