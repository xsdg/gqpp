#!/bin/sh

#/*
# * Copyright (C) 2022 The Geeqie Team
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
## for GTK3, and with and without optional modules.
## 

compile()
{
compiler="$1"

printf '\e[32m%s\n' "$compiler all disabled"
meson configure build -Darchive=disabled -Dcms=disabled -Ddjvu=disabled -Dexiv2=disabled -Dvideothumbnailer=disabled -Dgps-map=disabled -Dheif=disabled -Dj2k=disabled -Djpeg=disabled -Djpegxl=disabled -Dlibraw=disabled -Dlua=disabled -Dpdf=disabled -Dspell=disabled -Dtiff=disabled -Dwebp=disabled

if (! ninja -C build clean > /dev/null 2>&1)
then
	echo "ERROR"
fi
if (! ninja -C build > /dev/null 2>&1)
then
	echo "ERROR"
fi

printf '\e[32m%s\n' "$compiler none disabled"
meson configure build -Darchive=auto -Dcms=auto -Ddjvu=auto -Dexiv2=auto -Dvideothumbnailer=auto -Dgps-map=auto -Dheif=auto -Dj2k=auto -Djpeg=auto -Djpegxl=auto -Dlibraw=auto -Dlua=auto -Dpdf=auto -Dspell=auto -Dtiff=auto -Dwebp=auto

if (! ninja -C build clean > /dev/null 2>&1)
then
	echo "ERROR"
fi
if (! ninja -C build > /dev/null 2>&1)
then
	echo "ERROR"
fi
}

export CC=clang
export CXX=clang++
compile "clang"

export CC=
export CXX=
compile "gcc"
