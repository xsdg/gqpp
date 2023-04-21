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
## @brief Generate a list of people who have contributed translations.
##
## This list will be displayed in the About - Credits dialog.
##
## It is expected that the .po files have a list of translators preceded
## by the line "# Translators:" and terminated by a line containing only "#"
##

for file in "$1"/po/*.po
do
	base=$(basename "$file")
	locale=${base%.po}
	awk '$1 == "'"$locale"'" {print $0}' locales.txt
	awk '$2 ~/Translators:/ {
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
	}' "$file"

	printf "\n"
done > "$2"/translators
printf "\n\0" >> "$2"/translators
