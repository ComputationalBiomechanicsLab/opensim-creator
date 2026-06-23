from sphinx.ext.autodoc import FunctionDocumenter

project = "OPynSim"
copyright = "2026, Adam Kewley"
author = "Adam Kewley"
github_username = "adamkewley"
github_repository = "https://github.com/opynsim/opynsim"

numfig = True  # enable numbering each figure (e.g. "figure 1", "figure 2" in captions)

extensions = [
    "sphinx.ext.autodoc",       # For `autofunction::`, `automodule::`
    "sphinx.ext.autosummary",   # For `autosummary::`
    "sphinx.ext.napoleon",      # For parsing docstrings in the `opynsim` module
    "sphinx_copybutton",        # Adds a copy button to each codeblock (handy)
]

# `sphinx.ext.autodoc` settings:
autodoc_default_options = {
    'members': True,
    'undoc-members': True,
    'special-members': '__init__, __call__, __arrow_c_stream__',
    'imported-members': True,
    'inherited-members': True,  # IMPORTANT: this is how the parser finds `opynsim._core` nanobind functions
    'show-inheritance': True,
}
autodoc_use_legacy_class_based = True  # IMPORTANT: this is required to enable `Documenter` API, used to find nanobind functions
class NanobindFunctionDocumenter(FunctionDocumenter):
    """
    Specialized `Documenter` for `nanobind.nb_func` objects emitted
    from the `opynsim._core` module.
    """
    objtype = 'nanobindfunction'
    directivetype = 'function'                   # override emitting `autonanobindfunction::` with `autofunction::`
    priority = 10 + FunctionDocumenter.priority  # Check this documenter before the base function one

    @classmethod
    def can_document_member(cls, member, membername, isattr, parent):
        return "nanobind.nb_func" in str(type(member))

# `sphinx.ext.autosummary` settings:
autosummary_generate = True
autosummary_imported_members = True

# `sphinx.ext.napoleon` settings:
napoleon_google_docstring = True
napoleon_numpy_docstring = False  # `opynsim` uses Google-style docstrings

# html output options:
html_theme = 'sphinx_book_theme'
html_logo = '_static/opynsim_banner_vertical.svg'
html_favicon = '_static/opynsim_logo.svg'
html_static_path = ['_static']  # note: static files listed here are copied after builtin static files (overwrite)

def setup(app):  # Ran when Sphinx is setting itself up
    app.setup_extension("sphinx.ext.autodoc")
    app.add_autodocumenter(NanobindFunctionDocumenter, override=True)  # IMPORTANT: this is required to find nanobind functions
