# Makefile for Sphinx documentation

.PHONY: help html

srcdir        ?= .
SPHINXBUILD    = python3 -m sphinx
PAPER         ?= letter
BUILDDIR      ?= _build
JAVA          ?= java
PLANTUML_JAR  ?= $(HOME)/bin/plantuml.jar
PLANTUML_ARGS ?=
REALPATH       = $(if $(WINDIR), cygpath --absolute --windows, realpath)

SBUILD = $(SPHINXBUILD) ${PAPEROPT_letter}
PLANTUML = $(JAVA) -jar $(PLANTUML_JAR) $(PLANTUML_ARGS)
IMAGEDIR = $(srcdir)/pix
BUILD_IMAGEDIR = $(BUILDDIR)/html/_images
UMLDIR = $(srcdir)/uml

$(IMAGEDIR)/%.svg : $(UMLDIR)/%.uml
	$(PLANTUML) $< -tsvg -o $(shell $(REALPATH) $(IMAGEDIR))

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html       to make standalone HTML files"

html: uml ext/local-config.py
	$(SBUILD) -d $(BUILDDIR)/doctrees -b html $(srcdir) $(BUILDDIR)/html
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/html."

clean:
	-rm -rf html warn.log
	-rm -rf $(BUILDDIR)/doctrees $(BUILDDIR)/html $(BUILDDIR)/dirhtml $(BUILDDIR)/singlehtml

publish: clean html
	$(SHELL) ./publish.sh

ext/local-config.py : Makefile
	@echo "plantuml = '$(JAVA) -jar $(PLANTUML_JAR)'" > ext/local-config.py
	@echo "plantuml_output_format = 'svg'" >> ext/local-config.py
