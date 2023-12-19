## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
SUBDIRS = manuals

EXTRA_DIST = api/libabigail.doxy website/libabigail-website.doxy

html-doc: $(DO_HTML)
website: website-html-doxygen

if ENABLE_APIDOC
install-html-doc: $(DO_INSTALL_HTML)
endif

DO_HTML = apidoc-html-doxygen website-html-doxygen
DO_INSTALL_HTML = apidoc-install-html-doxygen

#doxygen
DOXY_API_SRC_CFG 	=	${abs_srcdir}/api/libabigail.doxy
DOXY_API_BLD_DIR 	=	${abs_builddir}/api
DOXY_API_BLD_CFG 	=	${abs_builddir}/api/html.doxy
DOXY_WEBSITE_BLD_DIR 	=	${abs_builddir}/website
DOXY_WEBSITE_SRC_CFG 	=	${abs_srcdir}/website/libabigail-website.doxy
DOXY_WEBSITE_BLD_CFG 	=	${abs_builddir}/website/libabigail-website.doxy

$(DOXY_API_BLD_DIR):
	if test ! -d $(DOXY_API_BLD_DIR); then		\
	  mkdir -p $(DOXY_API_BLD_DIR); 		\
	fi

$(DOXY_WEBSITE_BLD_DIR):
	if test ! -d $(DOXY_WEBSITE_BLD_DIR); then	\
	  mkdir -p $(DOXY_WEBSITE_BLD_DIR);		\
	fi

doxygen-api-cfg: $(DOXY_API_SRC_CFG) $(DOXY_API_BLD_DIR)
	sed -e "s=LIBABIGAIL_INPUT=${abs_srcdir}/..=g" < $(DOXY_API_SRC_CFG) \
	> $(DOXY_API_BLD_CFG)
	sed -i -e "s=LIBABIGAIL_OUTPUT=${abs_builddir}/..=g" $(DOXY_API_BLD_CFG)

doxygen-website-cfg: $(DOXY_WEBSITE_SRC_CFG) $(DOXY_WEBSITE_BLD_DIR)
	sed -e "s=LIBABIGAIL_INPUT=${abs_srcdir}/..=g" < $(DOXY_WEBSITE_SRC_CFG) \
	> $(DOXY_WEBSITE_BLD_CFG)
	sed -i -e "s=LIBABIGAIL_OUTPUT=${abs_builddir}/..=g" $(DOXY_WEBSITE_BLD_CFG)

apidoc-html-doxygen: doxygen-api-cfg
	doxygen $(DOXY_API_BLD_CFG)

website-html-doxygen: doxygen-website-cfg
	doxygen $(DOXY_WEBSITE_BLD_CFG)

apidoc-install-html-doxygen: apidoc-html-doxygen
	test -d "$(DESTDIR)$(docdir)" || $(mkinstalldirs) "$(DESTDIR)$(docdir)"
	cp -r ${DOXY_API_BLD_DIR} "$(DESTDIR)$(docdir)"

website-install-html-doxygen: doxygen-website-cfg
	test -z "$(DESTDIR)$(docdir)/website" || $(mkinstalldirs) "$(DESTDIR)$(docdir)/website"
	cp -r $(DOXY_WEBSITE_BLD_DIR) "$(DESTDIR)$(docdir)/website"

# By adding these files here, automake will remove them for 'make clean'
CLEANFILES = $(DOXY_API_BLD_CFG) $(DOXY_WEBSITE_BLD_CFG)

# To remove directories.
clean-local:
	-rm -rf $(DOXY_API_BLD_DIR)/*/* $(DOXY_WEBSITE_BLD_DIR)/*/*

#To install everything:
if ENABLE_APIDOC

install-html-doc: install-html

uninstall-local: clean-local
	-rm -rf $(DESTDIR)$(docdir)
endif
