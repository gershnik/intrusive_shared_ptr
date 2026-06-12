# Configuration file for the Sphinx documentation builder.
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import pathlib

# -- Project information -----------------------------------------------------

project = "intrusive_shared_ptr"
author = "Eugene Gershnik"
copyright = "2004, Eugene Gershnik"

# Version is read from the VERSION file at the repository root.
_version_file = pathlib.Path(__file__).resolve().parent.parent / "VERSION"
try:
    release = _version_file.read_text(encoding="utf-8").strip()
except OSError:
    release = ""
version = release

# -- General configuration ---------------------------------------------------

extensions = [
    "sphinx.ext.githubpages",  # writes .nojekyll into the output for GitHub Pages
]

exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# Show API entries in the page TOC (right sidebar) by their own name only, e.g.
# "load()" rather than "atomic::load()". Without this, members declared nested
# inside a class directive get a "Class::" prefix while namespace-scoped ones
# don't, which looks inconsistent.
toc_object_entries_show_parents = "hide"

# Unmarked literal blocks (::) and code-blocks without a language render as C++.
highlight_language = "cpp"
primary_domain = "cpp"

# -- HTML output -------------------------------------------------------------

html_theme = "furo"
#html_theme = "alabaster"

html_title = f"{project}".strip()
html_static_path = ["_static"]
html_css_files = ["custom.css"]
