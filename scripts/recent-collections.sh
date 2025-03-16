#! /bin/sh
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
## @brief List recently used Collections
##
## This may be useful if the user has Collections outside
## the default directory for Collections.
## Although the Recent Collections menu item has been deleted,
## the history list is still maintained.

awk -W posix '
BEGIN {
    LINT = "fatal"
    found = 0
}

/\[recent\]/ {
    found = 1
    next
}

/^$/ {
    found = 0
}

{
    if (found == 1) {
        print
    }
}
' "$HOME"/.config/geeqie/history | tr -d '"'
