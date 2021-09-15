
SUBDIRS = rotate symlink ufraw geocode-parameters export-jpeg tethered-photography camera-import image-crop random-image lens open-archive
qq_desktoptemplatedir = $(appdir)
qq_desktoptemplate_in_files = template.desktop.in
qq_desktoptemplate_DATA = $(qq_desktoptemplate_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(qq_desktoptemplate_in_files)
CLEANFILES = $(qq_desktoptemplate_DATA)
