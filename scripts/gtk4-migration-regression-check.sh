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
## @brief Look for function calls that do not conform to GTK4 migration format
##
## The files compat.cc and compat.h contain functions that should be used instead
## of the standard GTK calls. The compatibility calls are all prefixed by gq_
##
## Create a list of function names to be looked for by searching
## compat.cc and compat.h for identifiers prefixed by gq_gtk_
##
## Search the input file for any of the above gtk_ function names that
## are not prefixed by gq_
##
## $1 full path to file to process
## $2 full path to compat.cc
## $3 full path to compat.h
##

exit_status=0

if [ ! "${1#*compat.cc}" = "$1" ]
then
	exit "$exit_status"
fi

if [ ! "${1#*compat.h}" = "$1" ]
then
	exit "$exit_status"
fi

compat_functions=$(grep --only-matching --no-filename 'gq_gtk_\(\(\([[:alnum:]]*\)\_*\)*\)' "$2" "$3" | sort | uniq | cut --characters 4-)

while read -r line
do
	if grep --line-number --perl-regexp '(?<!gq_)'"$line" "$1"
	then
		exit_status=1
	fi
done << EOF
$compat_functions
EOF

exit "$exit_status"
