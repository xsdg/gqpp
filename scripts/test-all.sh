#!/bin/sh
#**********************************************************************
# Copyright (C) 2024 - The Geeqie Team
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
## @brief Run all tests
##
## Run test with all options disabled,
## and then with -Ddevel=enabled and other
## options as auto

if [ ! -d "src" ] || [ ! -f "geeqie.1" ]
then
	printf '%s\n' "This is not a Geeqie project folder"
	exit 1
fi

XDG_CONFIG_HOME=$(mktemp -d "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
XDG_CACHE_HOME=$(mktemp -d "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
XDG_DATA_HOME=$(mktemp -d "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
export XDG_CONFIG_HOME
export XDG_CACHE_HOME
export XDG_DATA_HOME

rm --recursive --force build
tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")

# Check with all options disabled
meson setup \
-Darchive=disabled \
-Dcms=disabled \
-Ddoxygen=disabled \
-Ddjvu=disabled \
-Devince=disabled \
-Dexecinfo=disabled \
-Dexiv2=disabled \
-Dexr=disabled \
-Dextended_stacktrace=disabled \
-Dfits=disabled \
-Dgit=disabled \
-Dgps-map=disabled \
-Dgtk4=disabled \
-Dheif=disabled \
-Dhelp_pdf=disabled \
-Dj2k=disabled \
-Djpeg=disabled \
-Djpegxl=disabled \
-Dlibraw=disabled \
-Dlua=disabled \
-Dnpy=disabled \
-Dpandoc=disabled \
-Dpdf=disabled \
-Dspell=disabled \
-Dtiff=disabled \
-Dunit_tests=disabled \
-Dvideothumbnailer=disabled \
-Dwebp=disabled \
-Dyelp-build=disabled \
build

if ninja test -C build
then
	options_disabled="PASS"
else
	options_disabled="FAIL"
fi

cp ./build/meson-logs/meson-log.txt "$tmpdir/testlog-options-disabled.txt"
cat ./build/meson-logs/testlog.txt >> "$tmpdir/testlog-options-disabled.txt"

rm --recursive --force build
meson setup -Dunit_tests=enabled build
if ninja test -C build
then
	options_enabled="PASS"
else
	options_enabled="FAIL"
fi

cp ./build/meson-logs/meson-log.txt "$tmpdir/testlog-options-enabled.txt"
cat ./build/meson-logs/testlog.txt >> "$tmpdir/testlog-options-enabled.txt"

rm -r "$XDG_CONFIG_HOME"
rm -r "$XDG_CACHE_HOME"
rm -r "$XDG_DATA_HOME"

printf "\n"
if [ "$options_disabled" = "PASS" ]
then
	printf "%s \033[1;32m PASS \033[0m\n" "$tmpdir/testlog-options-disabled.txt"
else
	printf "%s \033[1;31m FAIL \033[0m\n" "$tmpdir/testlog-options-disabled.txt"
fi

if [ "$options_enabled" = "PASS" ]
then
	printf "%s \033[1;32m PASS \033[0m\n" "$tmpdir/testlog-options-enabled.txt"
else
	printf "%s \033[1;31m FAIL \033[0m\n" "$tmpdir/testlog-options-enabled.txt"
fi

