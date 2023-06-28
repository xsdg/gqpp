#! /bin/sh
#**********************************************************************
# Copyright (C) 2022 - The Geeqie Team
#
# Author: Colin Clark
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#**********************************************************************

## @file
## @brief Generate a Geeqie AppImage.
##
## It must be run from the base Geeqie folder.  
## The single parameter is the directory where the AppDir
## will be created.
##

x86_64()
{
	linuxdeploy-x86_64.AppImage \
		--appdir ./AppDir \
		--output appimage \
		--desktop-file ./AppDir/usr/share/applications/geeqie.desktop \
		--icon-file ./AppDir/usr/share/pixmaps/geeqie.png \
		--plugin gtk \
		--executable ./AppDir/usr/bin/geeqie
}

## @FIXME arm AppImage of linuxdeploy does not yet exist
## Compile from sources
aarch64()
{
	linuxdeploy \
		--appdir ./AppDir \
		--output appimage \
		--desktop-file ./AppDir/usr/share/applications/geeqie.desktop \
		--icon-file ./AppDir/usr/share/pixmaps/geeqie.png \
		--plugin gtk \
		--executable ./AppDir/usr/bin/geeqie

	appimagetool-aarch64.AppImage ./AppDir/ ./Geeqie-aarch64.AppImage
}

if [ ! -f geeqie.spec.in ] || [ ! -d .git ]
then
	printf '%s\n' "This is not a Geeqie folder"
	exit 1
fi

if ! target_dir=$(realpath "$1")
then
	printf '%s\n' "No target dir specified"
	exit 1
fi

rm -rf ./build-appimge
rm -rf "$target_dir"/AppDir
mkdir "$target_dir"/AppDir || {
	printf '%s\n' "Cannot make $target_dir/AppDir"
	exit 1
}

meson setup build-appimage
meson configure build-appimage -Dprefix="/usr/"
DESTDIR=/"$target_dir"/AppDir ninja -C build-appimage install

VERSION=$(git tag | tail -1)
export VERSION

cd "$target_dir" || {
	printf '%s\n' "Cannot cd to $target_dir"
	exit 1
}

case $(uname -m) in
	"x86_64")
		x86_64

		new_version="$(./Geeqie-"$VERSION"-x86_64.AppImage -v | sed 's/git//' | sed 's/-.* /-/' | sed 's/ /-v/' | sed 's/-GTK3//')-x86_64.AppImage"
		mv "./Geeqie-$VERSION-x86_64.AppImage" "$new_version"
		gpg --armor --detach-sign --output "$new_version.asc" "$new_version"
		;;

	"aarch64")
		aarch64

		new_version="$(./Geeqie-aarch64.AppImage -v | sed 's/git//' | sed 's/-.* /-/' | sed 's/ /-v/' | sed 's/-GTK3//')-aarch64.AppImage"
		mv "./Geeqie-aarch64.AppImage" "$new_version"
		gpg --armor --detach-sign --output "$new_version.asc" "$new_version"
		;;

	*)
		printf "Architecture unknown"
		;;
esac
