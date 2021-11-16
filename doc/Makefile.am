## Process this file with automake to produce Makefile.in.

helpdir = @htmldir@

EXTRA_DIST = docbook2html.sh docbook icons

html/GuideIndex.html: docbook/GuideIndex.xml
	rm -rf html; mkdir html; cp $(srcdir)/icons/* html/; ./create-doxygen-lua-api.sh
	if [ -x "$(GNOME_DOC_TOOL)" ]; then \
		"$(GNOME_DOC_TOOL)" html -i -o html/ $(srcdir)/docbook/GuideIndex.xml ; \
	else \
		echo "yelp-tools not found, html is not built" ; \
	fi

html: html/GuideIndex.html

install-data-hook: html
	if [ -x "$(GNOME_DOC_TOOL)" ]; then \
		$(MKDIR_P) "$(DESTDIR)$(helpdir)" || exit 1; \
		cd $(srcdir)/html; for f in * ; do $(INSTALL_DATA) "$$f" "$(DESTDIR)$(helpdir)/$$f" ; done; \
		ln -s -f GuideIndex.html "$(DESTDIR)$(helpdir)/index.html" ; \
		$(MKDIR_P) "$(DESTDIR)$(helpdir)/lua-api" || exit 1; \
		$(MKDIR_P) "$(DESTDIR)$(helpdir)/lua-api/html" || exit 1; \
		cd ./lua-api/html; for f in * ; do $(INSTALL_DATA) "$$f" "$(DESTDIR)$(helpdir)/lua-api/html/$$f" ; done; \
	fi

uninstall-hook:
	rm -r "$(DESTDIR)$(helpdir)/"*

clean-local:
	rm -rf html
