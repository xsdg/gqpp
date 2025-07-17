#!/bin/sh
#**********************************************************************
# Copyright (C) 2025 - The Geeqie Team
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
## @brief Used as the parameter for linuxdeploy --custom-apprun=[AppRun path]
##
## Used when creating an AppImage.
## It solves problems resulting when the Geeqie AppImage is called via a symlink.
##
## Note that the file created by linuxdeploy --custom-apprun= is AppRun.wrapped and not AppRun.
## AppRun is still created by linuxdeploy. It calls AppRun.wrapped which
## originally is merely a symlink to the executable. This file replaces the symlink.
##
## See .github/workflows/appimage*
##

# make sure errors in sourced scripts will cause this script to stop
set -e

this_dir=$(dirname "$(readlink -f "$0")")

# shellcheck disable=SC1091
. "$this_dir/apprun-hooks/linuxdeploy-plugin-gtk.sh"

exec "$this_dir/usr/bin/geeqie" "$@"
