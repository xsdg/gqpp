#!/bin/sh
#**********************************************************************
# Copyright (C) 2023 - The Geeqie Team
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
## @brief Run doxygen for the Geeqie project
##
## The environment variables needed to generate the
## Geeqie doxygen documentation are set up here.
##
## Run from the project root directory.
##
## $1 Destination directory. If blank, default is ../doxygen
##

if ! { [ -d ".git" ] && [ -d "src" ] && [ -f "geeqie.1" ] && [ -f "doxygen.conf" ]; }
then
	printf "This is not the project root directory\n"
    exit
fi

if [ -n "$1" ]
then
	DOCDIR="$1"
else
	DOCDIR="$PWD"/../doxygen
fi

if ! mkdir -p "$DOCDIR"/
then
	printf "Cannot create %s\n" "$DOCDIR"
	exit 1
fi

export DOCDIR
export SRCDIR="$PWD"
export PROJECT="Geeqie"
VERSION=$(git -C "$PWD" tag --list v[1-9]* | tail -n 1)
export VERSION
export PLANTUML_JAR_PATH="$HOME"/bin/plantuml.jar
export INLINE_SOURCES=YES
export STRIP_CODE_COMMENTS=NO

# Set doxygen.conf parameters so that searchdata.xml is generated
EXTERNAL_SEARCH="YES"
SERVER_BASED_SEARCH="YES"
export EXTERNAL_SEARCH
export SERVER_BASED_SEARCH

doxygen doxygen.conf

tmp_searchdata_xml=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
mv "$DOCDIR/searchdata.xml" "$tmp_searchdata_xml"

# Run again with default settings so that the html search box is generated
EXTERNAL_SEARCH="NO"
SERVER_BASED_SEARCH="NO"

doxygen doxygen.conf

mv "$tmp_searchdata_xml" "$DOCDIR/searchdata.xml"
