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
## @brief Update pixbuf loaders.cache file
##
## Required only for webp-pixbuf-loader subproject, and
## only when Geeqie is being installed
##

loader=$(find /usr/local -name libpixbufloader-webp.so)
dest=$(pkg-config gdk-pixbuf-2.0 --variable=gdk_pixbuf_moduledir)

sudo cp "$loader" "$dest"

sudo "$(pkg-config gdk-pixbuf-2.0 --variable=gdk_pixbuf_query_loaders)" --update-cache
