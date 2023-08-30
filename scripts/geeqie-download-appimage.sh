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
## @brief Download full and minimal AppImages from the Continuous build release on GitHub.
##
## The user should modify the symbolic links as appropriate.
##
## Downloads will not be made unless the server version is newer than the local file.
##

cd "$HOME/bin/" || exit
architecture=$(arch)

wget --quiet --show-progress --backups=3 --timestamping "https://github.com/BestImageViewer/geeqie/releases/download/continuous/Geeqie-latest-$architecture.AppImage";
chmod +x "Geeqie-latest-$architecture.AppImage"
ln --symbolic --force "Geeqie-latest-$architecture.AppImage" Geeqie

wget --quiet --show-progress --backups=3 --timestamping "https://github.com/BestImageViewer/geeqie/releases/download/continuous/Geeqie-minimal-latest-$architecture.AppImage";
chmod +x "Geeqie-minimal-latest-$architecture.AppImage"
ln --symbolic --force "Geeqie-minimal-latest-$architecture.AppImage" geeqie
