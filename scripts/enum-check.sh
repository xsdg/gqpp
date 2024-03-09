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
## @brief Check for single value enums
##
## $1 Source file to check
##
## The expected format is: \n
## enum { \n
##  bookmark_drop_types_n = 3 \n
## }; \n
##
## The grep operation prepends line numbers so resulting in: \n
## 123:enum { \n
## 124- bookmark_drop_types_n = 3 \n
## 125-};
##

res=$(grep --line-number --after-context=2 'enum\ {$' "$1" | grep --before-context=2 '^[[:digit:]]\+\-};')

if [ -z "$res" ]
then
	exit 0
else
	printf "%s\n" "$res"
	exit 1
fi

