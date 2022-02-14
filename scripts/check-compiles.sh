#!/bin/bash

#/*
# * Copyright (C) 2021 The Geeqie Team
# *
# * Author: Colin Clark  
# *  
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License along
# * with this program; if not, write to the Free Software Foundation, Inc.,
# * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# */

## @file
## @brief Check that Geeqie compiles with both gcc and clang,
## for both GTK2 and GTK3, and with and without optional modules.
## 

compile()
{
	# Cannot have --enable-debug-flags with --disable-gtk3

	declare -A variant
	variant[0]="$disable_list --disable-gtk3"
	variant[1]="--disable-gtk3"
	variant[2]="--enable-debug-flags $disable_list"
	variant[3]="--enable-debug-flags"
	variant[4]="$disable_list"
	variant[5]=""

	for ((i = 0; i <= 5; i++))
	do
		if [[ "${variant[$i]}" =~ "gtk3" ]]; then
			gtk="GTK2"
		else
			gtk="GTK3"
		fi
		if [[ "${variant[$i]}" =~ "disable-threads" ]]; then
			disabled="all disabled"
		else
			disabled="none disabled"
		fi
		if [[ "${variant[$i]}" =~ "--enable-debug-flags" ]]; then
			debug_flags="enable-debug-flags"
		else
			debug_flags=""
		fi

		echo -e " \e[32m $1 $gtk $debug_flags $disabled "
		sudo make maintainer-clean > /dev/null 2>&1
		./autogen.sh "${variant[$i]}" > /dev/null 2>&1
		make -j > /dev/null
	done
}

disable_list=" "$(awk --field-separator '[][]' '/AC_HELP_STRING\(\[--disable-/ {if ($2 != "gtk3") print $2}' configure.ac | tr '\n' ' ')

echo "Disabled list: :$disable_list"

export CFLAGS="-Wno-deprecated-declarations"

export CC=clang
export CXX=clang++
compile "clang"

export CC=
export CXX=
compile "gcc"
