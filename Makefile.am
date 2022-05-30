## Process this file with automake to produce Makefile.in.

include $(top_srcdir)/aminclude.am

SUBDIRS = src tests po doc plugins
DIST_SUBDIRS = src po doc plugins

man_MANS = geeqie.1

readmedir = @readmedir@

if HAVE_PANDOC
readme_DATA = README.md COPYING ChangeLog TODO README.lirc AUTHORS README.html ChangeLog.html
else
readme_DATA = README.md COPYING ChangeLog TODO README.lirc AUTHORS ChangeLog.html
endif

desktopdir = $(datadir)/applications
desktop_in_files = geeqie.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

icondir = $(datadir)/pixmaps
icon_DATA = geeqie.png

# Location defined by freedesktop.org
if EXTERNAL_PREFIX
appdatadir = $(datadir)/metainfo
else
appdatadir = /usr/share/metainfo
endif
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = org.geeqie.Geeqie.appdata.xml.in
@INTLTOOL_XML_RULE@

EXTRA_DIST = \
	$(readme_DATA)	\
	$(desktop_in_files)	\
	$(icon_DATA)	\
	geeqie.spec.in	\
	geeqie.1	\
	doxygen.conf	\
	$(appdata_in_files)

dist-hook: geeqie.spec
	cp $(top_builddir)/geeqie.spec $(distdir)

DISTCLEANFILES = config.report
CLEANFILES = $(desktop_DATA) ChangeLog.html $(appdata_DATA)

.PHONY: ChangeLog
ChangeLog.html:
	./gen_changelog.sh

README.html: README.md
	./gen_readme.sh
