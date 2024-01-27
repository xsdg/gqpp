#! /bin/sh
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
## @brief Use desktop-file-validate on .desktop files.
##
## $1 temp directory \n
## $2 desktop file name including .in extension \n
##
## desktop-file-validate will not process a file with
## the extension ".in". Use a symlink as a workaround.

if [ ! -d "$1" ]
then
	mkdir --parents "$1"
fi

desktop_file=$(basename "$2" ".in")

ln --symbolic "$2" "$1/$desktop_file"

result=$(desktop-file-validate "$1/$desktop_file")

rm "$1/$desktop_file"

if [ -z "$result" ]
then
	exit 0
else
	printf "%s\n" "$result"

	exit 1
fi

