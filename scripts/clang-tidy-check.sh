#! /bin/sh
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
## @brief Run clang-tidy on all source files.
##

if [ ! -d ".git" ] || [ ! -d "src" ] || [ ! -f "geeqie.1" ]
then
	printf '%s\n' "This is not a Geeqie project folder"
	exit 1
fi

i=0

secs_start=$(cut --delimiter='.' --fields=1 < /proc/uptime)
total=$(find src -name "*.cc" | wc --lines)

while read -r file
do
	i=$((i + 1))
	printf '%d/%d %s\n' "$i" "$total" "$file"
	clang-tidy --config-file ./.clang-tidy -p ./build "$file" 2>/dev/null

	secs_now=$(cut --delimiter='.' --fields=1 < /proc/uptime)

	elapsed_time=$(( secs_now - secs_start ))
	remaining_files=$(( total - i ))
	average_time=$(( elapsed_time / i ))
	estimated=$(( average_time * remaining_files ))

	printf 'Remaining: %dm:%ds\n' $((estimated%3600/60)) $((estimated%60))
done << EOF
$(find src -name "*.cc")
EOF
