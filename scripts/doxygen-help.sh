#!/bin/bash

#**********************************************************************
# Copyright (C) 2021 - The Geeqie Team
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
#
# Facilitate the integration of the Doxygen html files
# into a code editor.
#
# doxygen-help.sh <parameter to be searched for>
#
# The environment variable DOCDIR must be set to point to the
# Doxygen html files location.
# The environment variable PROJECT must be set to the value used
# when creating the Doxygen files.
#
# Set a hot key in the code editor to call this script with
# the highlighted variable or function name as a parameter.
#
# The file $DOCDIR/$PROJECT.tag contains an index of all documented
# items and is scanned to locate the relevant html section.
#
# xdg-open is used to call the html browser to display the document.
#
#**********************************************************************
#
# To generate the Doxygen html files set the following
# environment variables:
# DOCDIR (destination folder)
# SRCDIR (must point to the level above the source code)
# PROJECT
# VERSION
#
# Then run 'doxygen doxygen.conf'

if [[ -z "${DOCDIR}" ]]
then
	echo "Environment variable DOCDIR not set"
	zenity --title="Geeqie" --width=200 --warning --text="Environment variable DOCDIR not set"
elif [[ -z "${PROJECT}" ]]
then
	echo "Environment variable PROJECT not set"
	zenity --title="Geeqie" --width=200 --warning --text="Environment variable PROJECT not set"
else
	awk  -v search_param="$1" -v docdir="$DOCDIR" '
		{
		if ($1 ==  "<name>_"search_param"</name>")
			{
			found=0
			while (found == 0)
				{
				getline
				n=split($1, anchorfile, /[<>]/)
				if (anchorfile[2] == "anchorfile")
					{
					found=1
					}
				}
			data_result="file://"docdir"/html/" anchorfile[3]
			}
		else
			{
			if ($1 == "<name>"search_param"</name>")
				{
				getline
				n=split($1, anchorfile, /[<>]/)

				getline
				n=split($1, anchor, /[<>]/)
				function_result="file://"docdir"/html/" anchorfile[3] "#" anchor[3]
				}
			}
		}
		END {
			if (data_result != "")
				{
				print data_result
				}
			else if (function_result != "")
				{
				print function_result
				}
			}
		' $DOCDIR/$PROJECT.tag | while read -r file; do xdg-open "$file"; done
fi
