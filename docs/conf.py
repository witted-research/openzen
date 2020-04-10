# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Project information -----------------------------------------------------

project = 'OpenZen'
copyright = '2020, LP-Research Inc.'
author = 'LP-Research Inc.'

master_doc = 'index'

# use a much nicer theme
html_theme = 'sphinx_rtd_theme'
# use the theme from our code repository because it is not installed by default any more
html_theme_path = ["_themes", ]
