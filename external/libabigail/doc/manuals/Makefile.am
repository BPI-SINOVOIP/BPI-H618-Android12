## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Makefile for Sphinx documentation

EXTRA_DIST = \
abidiff.rst \
abipkgdiff.rst \
abicompat.rst \
abidw.rst \
abilint.rst \
conf.py \
index.rst \
libabigail-concepts.rst \
libabigail-overview.rst \
libabigail-tools.rst \
fedabipkgdiff.rst \
kmidiff.rst

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
PAPER         =
BUILDDIR      = $(abs_builddir)
SOURCEDIR     = $(abs_srcdir)

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = -d $(BUILDDIR)/doctrees $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) $(SOURCEDIR)
# the i18n builder cannot share the environment and doctrees with the others
I18NSPHINXOPTS  = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .

.PHONY: help clean html-local dirhtml singlehtml pickle json htmlhelp qthelp devhelp epub latex latexpdf text man changes linkcheck doctest gettext

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html-doc   to make standalone HTML files"
	@echo "  dirhtml    to make HTML files named index.html in directories"
	@echo "  singlehtml to make a single large HTML file"
	@echo "  pickle     to make pickle files"
	@echo "  json       to make JSON files"
	@echo "  htmlhelp   to make HTML files and a HTML help project"
	@echo "  qthelp     to make HTML files and a qthelp project"
	@echo "  devhelp    to make HTML files and a Devhelp project"
	@echo "  epub       to make an epub"
	@echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  latexpdf   to make LaTeX files and run them through pdflatex"
	@echo "  text       to make text files"
	@echo "  man        to make manual pages"
	@echo "  texinfo    to make Texinfo files"
	@echo "  info       to make Texinfo files and run them through makeinfo"
	@echo "  gettext    to make PO message catalogs"
	@echo "  changes    to make an overview of all changed/added/deprecated items"
	@echo "  linkcheck  to check all external links for integrity"
	@echo "  doctest    to run all doctests embedded in the documentation (if enabled)"

html-doc:
	$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) $(DESTDIR)$(BUILDDIR)/html
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/html."

dirhtml:
	$(SPHINXBUILD) -b dirhtml $(ALLSPHINXOPTS) $(BUILDDIR)/dirhtml
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/dirhtml."

singlehtml:
	$(SPHINXBUILD) -b singlehtml $(ALLSPHINXOPTS) $(BUILDDIR)/singlehtml
	@echo
	@echo "Build finished. The HTML page is in $(BUILDDIR)/singlehtml."

pickle:
	$(SPHINXBUILD) -b pickle $(ALLSPHINXOPTS) $(BUILDDIR)/pickle
	@echo
	@echo "Build finished; now you can process the pickle files."

json:
	$(SPHINXBUILD) -b json $(ALLSPHINXOPTS) $(BUILDDIR)/json
	@echo
	@echo "Build finished; now you can process the JSON files."

htmlhelp:
	$(SPHINXBUILD) -b htmlhelp $(ALLSPHINXOPTS) $(BUILDDIR)/htmlhelp
	@echo
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      ".hhp project file in $(BUILDDIR)/htmlhelp."

qthelp:
	$(SPHINXBUILD) -b qthelp $(ALLSPHINXOPTS) $(BUILDDIR)/qthelp
	@echo
	@echo "Build finished; now you can run "qcollectiongenerator" with the" \
	      ".qhcp project file in $(BUILDDIR)/qthelp, like this:"
	@echo "# qcollectiongenerator $(BUILDDIR)/qthelp/Libabigailmanuals.qhcp"
	@echo "To view the help file:"
	@echo "# assistant -collectionFile $(BUILDDIR)/qthelp/Libabigailmanuals.qhc"

devhelp:
	$(SPHINXBUILD) -b devhelp $(ALLSPHINXOPTS) $(BUILDDIR)/devhelp
	@echo
	@echo "Build finished."
	@echo "To view the help file:"
	@echo "# mkdir -p $$HOME/.local/share/devhelp/Libabigailmanuals"
	@echo "# ln -s $(BUILDDIR)/devhelp $$HOME/.local/share/devhelp/Libabigailmanuals"
	@echo "# devhelp"

epub:
	$(SPHINXBUILD) -b epub $(ALLSPHINXOPTS) $(BUILDDIR)/epub
	@echo
	@echo "Build finished. The epub file is in $(BUILDDIR)/epub."

latex:
	$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex
	@echo
	@echo "Build finished; the LaTeX files are in $(BUILDDIR)/latex."
	@echo "Run \`make' in that directory to run these through (pdf)latex" \
	      "(use \`make latexpdf' here to do that automatically)."

latexpdf:
	$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex
	@echo "Running LaTeX files through pdflatex..."
	$(MAKE) -C $(BUILDDIR)/latex all-pdf
	@echo "pdflatex finished; the PDF files are in $(BUILDDIR)/latex."

text:
	$(SPHINXBUILD) -b text $(ALLSPHINXOPTS) $(BUILDDIR)/text
	@echo
	@echo "Build finished. The text files are in $(BUILDDIR)/text."

man:
	$(SPHINXBUILD) -b man $(ALLSPHINXOPTS) $(BUILDDIR)/man
	@echo
	@echo "Build finished. The manual pages are in $(BUILDDIR)/man."

texinfo:
	$(SPHINXBUILD) -b texinfo $(ALLSPHINXOPTS) $(BUILDDIR)/texinfo
	@echo
	@echo "Build finished. The Texinfo files are in $(BUILDDIR)/texinfo."
	@echo "Run \`make' in that directory to run these through makeinfo" \
	      "(use \`make info' here to do that automatically)."

info:
	$(SPHINXBUILD) -b texinfo $(ALLSPHINXOPTS) $(BUILDDIR)/texinfo
	@echo "Running Texinfo files through makeinfo..."
	make -C $(BUILDDIR)/texinfo info
	@echo "makeinfo finished; the Info files are in $(BUILDDIR)/texinfo."

gettext:
	$(SPHINXBUILD) -b gettext $(I18NSPHINXOPTS) $(BUILDDIR)/locale
	@echo
	@echo "Build finished. The message catalogs are in $(BUILDDIR)/locale."

changes:
	$(SPHINXBUILD) -b changes $(ALLSPHINXOPTS) $(BUILDDIR)/changes
	@echo
	@echo "The overview file is in $(BUILDDIR)/changes."

linkcheck:
	$(SPHINXBUILD) -b linkcheck $(ALLSPHINXOPTS) $(BUILDDIR)/linkcheck
	@echo
	@echo "Link check complete; look for any errors in the above output " \
	      "or in $(BUILDDIR)/linkcheck/output.txt."

doctest:
	$(SPHINXBUILD) -b doctest $(ALLSPHINXOPTS) $(BUILDDIR)/doctest
	@echo "Testing of doctests in the sources finished, look at the " \
	      "results in $(BUILDDIR)/doctest/output.txt."

section1_manpages =
section7_manpages =
manpages =
texinfodocs =

if ENABLE_MANUAL

section1_manpages += abipkgdiff.1 abidiff.1 abidw.1 abilint.1 abicompat.1
section7_manpages += libabigail.7
manpages +=  $(section1_manpages) $(section7_manpages)
texinfodocs += abigail.info

install-html-doc: html-doc
	test -d "$(DESTDIR)$(docdir)/manual" || $(mkinstalldirs) "$(DESTDIR)$(docdir)/manual"
	cp -r "$(DESTDIR)$(BUILDDIR)/html" "$(DESTDIR)$(docdir)/manual"
else

install-html-doc:

endif

clean-local:
	-rm -rf $(BUILDDIR)/html
	-rm -rf $(BUILDDIR)/doctrees

install-man-and-info-doc:
	-test -f $(BUILDDIR)/man/abidiff.1 && $(mkinstalldirs) $(DESTDIR)$(mandir)/man1
	-test -f $(BUILDDIR)/man/libabigail.7 && $(mkinstalldirs) $(DESTDIR)$(mandir)/man7
	-for m in $(section1_manpages); do test -f $(BUILDDIR)/man/$$m && $(install_sh_DATA) $(BUILDDIR)/man/$$m $(DESTDIR)$(mandir)/man1; done;
	-for m in $(section7_manpages); do test -f $(BUILDDIR)/man/$$m && $(install_sh_DATA) $(BUILDDIR)/man/$$m $(DESTDIR)$(mandir)/man7; done;

	-test -f $(BUILDDIR)/texinfo/abigail.info && $(mkinstalldirs) $(DESTDIR)$(infodir)
	-for i in $(texinfodocs); do test -f $(BUILDDIR)/texinfo/$$i && $(install_sh_DATA) $(BUILDDIR)/texinfo/$$i $(DESTDIR)$(infodir)/$$i; gzip -f $(DESTDIR)$(infodir)/$$i; done;

uninstall-local: clean-local
	-for i in $(section1_manpages); do rm -f $(DESTDIR)$(mandir)/man1/$$i; done
	-for i in $(section7_manpages); do rm -f $(DESTDIR)$(mandir)/man7/$$i; done
	-for i in $(texinfodocs); do rm -f $(DESTDIR)$(infodir)/$$i; done
	-rm -rf $(DESTDIR)$(docdir)/manual/html
