#! /bin/sh
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
## @brief Generate lists of people who have made commits to the repository.
##
## The lists will be displayed in the About - Credits dialog.
##
## $1 Meson PRIVATE_DIR \n
## $2 Meson current_source_dir \n
## $3 Meson current_build_dir \n
## $4 locales.txt \n
## $5 gresource.xml \n
## $6...$n po source list - space separated list \n
##
## It is expected that the .po files have a list of translators in the form: \n
## \# Translators: \n
## \# translator1_name <translator1 email> \n
## \# translator2_name <translator2 email> \n
## \#

mkdir -p "$1"
private_dir="$1"
shift
source_dir="$1"
shift
build_dir="$1"
shift
locales="$1"
shift
resource_xml="$1"
shift

while [ -n "$1" ]
do
	base=$(basename "$1")
	full_file_path="$source_dir/$1"
	locale=${base%.po}

	printf "\n"
	awk '$1 == "'"$locale"'" {print $0}' "$locales"
	awk '$0 ~/Translators:/ {
		while (1) {
			getline $0
		if ($0 == "#") {
			exit
			}
		else {
			print substr($0, 3)
			}
		}
		print $0
	}' "$full_file_path"

shift
done > "$private_dir"/translators
printf "\n\0" >> "$private_dir"/translators

glib-compile-resources --generate-header --sourcedir="$private_dir" --target="$build_dir"/translators.h "$resource_xml"
glib-compile-resources --generate-source --sourcedir="$private_dir" --target="$build_dir"/translators.c "$resource_xml"
