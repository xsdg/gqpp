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
## @brief Perform validity checks on project ancillary files
##
## $1 Root of project sources
##
## Perform validity checks on project ancillary files:
## appdata
## desktop
## scripts
## ui
## xml
##

cd "$1" || exit 1

if [ ! -d "src" ] || [ ! -f "geeqie.1" ]
then
	printf '%s\n' "This is not a Geeqie project folder"
	exit 1
fi

exit_status=0

# Script files must have the file extension .sh  or
# be symlinked as so - for doxygen
while read -r file
do
	#~ check_sh
	result=$(file "$file" | grep "POSIX shell script")

	if [ -n "$result" ]
	then
		base_with_sh=$(basename "$file")
		base_without_sh=$(basename "$file" .sh)

		if [ "$base_with_sh" = "$base_without_sh" ]
		then
			if [ ! -f "$file.sh" ]
			then
				printf "ERROR; Executable script does not have a .sh extension: %s\n" "$file"
				exit_status=1
			fi
		fi
	fi
done << EOF
$(find "$1/plugins" "$1/src" "$1/scripts" -type f -executable)
EOF

# Check if all options are in the disabled checks
while read -r line
do
	if [ -n "$line" ]
	then
		res=$(grep "$line" "$1/scripts/test-all.sh")
		if [ -z "$res" ]
		then
			printf "ERROR; Option no disabled check in ./scripts/test-all.sh: %s\n" "$line"
			exit_status=1
		fi
	fi
done << EOF
$(awk 'BEGIN {FS="\047"} /option/ { if (substr($2,0,2) != "gq") { print $2 } }' meson_options.txt)
EOF

# Check if all options are in the disabled checks in a GitHub run
# Directory .github is not in the source tar
if [ -d ".github" ]
then
	while read -r line
	do
		if [ -n "$line" ]
		then
			res=$(grep "\-D$line=disabled" "$1/.github/workflows/check-build-actions.yml")
			if [ -z "$res" ]
			then
				printf "ERROR; Option no disabled check in .github/workflows/check-build-actions.yml: %s\n" "$line"
				exit_status=1
			fi
		fi
	done << EOF
$(awk 'BEGIN {FS="\047"} /option/ { if (substr($2,0,2) != "gq") { print $2 } }' meson_options.txt)
EOF
fi

# Markdown lint
# Runs as a GitHub Action
if [ -z "$GITHUB_WORKSPACE" ]
then
	if [ -z "$(command -v mdl)" ]
	then
		printf "ERROR: mdl is not installed"
		exit_status=1
	else
		while read -r line
		do
			if [ -n "$line" ]
			then
				if [ "${line#*": MD"}" != "$line" ]
				then
					printf "ERROR; Markdown lint error in: %s\n" "$line"
					exit_status=1
				fi
			fi
		done << EOF
$(find . -not -path "*/.*" -name "*.md" -exec mdl --no-verbose --config .mdlrc {} \;)
EOF
	fi
fi

# Shellcheck lint
# Runs as a GitHub Action
if [ -z "$GITHUB_WORKSPACE" ]
then
	if [ -z "$(command -v shellcheck)" ]
	then
		printf "ERROR: shellcheck is not installed"
		exit_status=1
	else
		while read -r line
		do
			if [ -n "$line" ]
			then
				shellcheck_error=$(shellcheck "$line" 2>&1)
				if [ -n "$shellcheck_error" ]
				then
					printf "ERROR; shellcheck error in: %s\n" "$shellcheck_error"
					exit_status=1
				fi
			fi
		done << EOF
$(find . -name "*.sh")
EOF
	fi
fi

# gtk-builder ui lint - should not check the menu.ui files
if [ -z "$(command -v gtk-builder-tool)" ]
then
	printf "ERROR: gtk-builder-tool is not installed"
	exit_status=1
else
	while read -r line
	do
		if [ -n "$line" ]
		then
			if [ "${line#*"menu"}" = "$line" ]
			then
				if [ -z "$GITHUB_WORKSPACE" ]
				then
					builder_error=$(gtk-builder-tool validate "$line" 2>&1)
					if [ -n "$builder_error" ]
					then
						printf "ERROR; gtk-builder-tool error in: %s\n" "$builder_error"
						exit_status=1
					fi
				else
					builder_error=$(xvfb-run --auto-servernum gtk-builder-tool validate "$line" 2>&1)
					if [ -n "$builder_error" ]
					then
						printf "ERROR; gtk-builder-tool error in: %s\n" "$builder_error"
						exit_status=1
					fi
				fi
			fi
		fi
	done << EOF
$(find $! -name "*.ui")
EOF
fi

# Desktop files lint
if [ -z "$(command -v desktop-file-validate)" ]
then
	printf "ERROR: desktop-file-validate is not installed"
	exit_status=1
else
	while read -r line
	do
		if [ -n "$line" ]
		then
			desktop_file=$(basename "$line" ".in")
			ln --symbolic "$line" "$1/$desktop_file"
			result=$(desktop-file-validate "$1/$desktop_file")

			rm "$1/$desktop_file"
			if [ -n "$result" ]
			then
				printf "ERROR; desktop-file-validate error in: %s %s\n" "$line" "$result"
				exit_status=1
			fi
		fi
	done << EOF
$(find . -name "*.desktop.in")
EOF
fi

# Appdata lint
if [ -z "$(command -v appstreamcli)" ]
then
	printf "ERROR: appstreamcli is not installed"
	exit_status=1
else
	if ! result=$(appstreamcli validate org.geeqie.Geeqie.appdata.xml.in --pedantic --explain)
	then
		exit_status=1
		status="Error"
	else
		line_count=$(echo "$result" | wc --lines)

		if [ "$line_count" -gt 1 ]
		then
			status="Warning"
		else
			status="Passed"
		fi
	fi

	printf "%s: appstreamcli in org.geeqie.Geeqie.appdata.xml.in: \n%s\n" "$status" "$result"
fi

# xml files lint
if [ -z "$(command -v xmllint)" ]
then
	printf "ERROR: xmllint is not installed"
	exit_status=1
else
	while read -r line
	do
		if [ -n "$line" ]
		then
			if ! xmllint --quiet --nowarning "$line" > /dev/null
			then
				printf "ERROR: xmllint error in: %s\n" "$line"
				exit_status=1
			fi
		fi
	done << EOF
$(find ./doc/docbook -name "*.xml")
EOF
fi

exit "$exit_status"
