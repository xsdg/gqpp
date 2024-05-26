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
## $1 Root of project sources or NULL for current
##
## Perform validity checks on project ancillary files:
## appdata
## desktop
## scripts
## ui
## xml
##

if [ -n "$1" ]
then
	cd "$1" || exit 1
fi

if [ ! -d "src" ] || [ ! -f "geeqie.1" ]
then
	printf '%s\n' "This is not a Geeqie project folder"
	exit 1
fi

exit_status=0

# All script files must be POSIX
# downsize is a third-party file and is excluded
while read -r file
do
	result=$(file "$file" | grep "shell script")

	if [ -n "$result" ]
	then
		if [ "${result#*"POSIX"}" = "$result" ]
		then
			printf "ERROR; Executable script is not POSIX: %s\n" "$file"
			exit_status=1
		fi
	fi
done << EOF
$(find "./plugins" "./src" "./scripts" -type f -not -name downsize -executable)
EOF

# Script files must have the file extension .sh  or
# be symlinked as so - for doxygen
while read -r file
do
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
$(find "./plugins" "./src" "./scripts" -type f -executable)
EOF

# Check if all options are in the disabled checks
while read -r line
do
	if [ -n "$line" ]
	then
		res=$(grep "$line" "./scripts/test-all.sh")
		if [ -z "$res" ]
		then
			printf "ERROR; Option no disabled check in ./scripts/test-all.sh: %s\n" "$line"
			exit_status=1
		fi
	fi
done << EOF
$(awk --lint=fatal --posix 'BEGIN {FS="\047"} /option\(/ { if (substr($2, 1, 2) != "gq") { print $2 } }' meson_options.txt)
EOF

# Check if all options are in the disabled checks in a GitHub run
# Directory .github is not in the source tar
if [ -d ".github" ]
then
	while read -r line
	do
		if [ -n "$line" ]
		then
			res=$(grep "\-D$line=disabled" "./.github/workflows/check-build-actions.yml")
			if [ -z "$res" ]
			then
				printf "ERROR; Option no disabled check in .github/workflows/check-build-actions.yml: %s\n" "$line"
				exit_status=1
			fi
		fi
	done << EOF
$(awk --lint=fatal --posix 'BEGIN {FS="\047"} /option\(/ { if (substr($2, 1, 2) != "gq") { print $2 } }' meson_options.txt)
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
$(find . -not -path "*/.*" -not -path "*/subprojects/*" -name "*.md" -exec mdl --no-verbose --config .mdlrc {} \;)
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
$(find . -name "*.sh" -not -path "./subprojects/*")
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
			ln --symbolic "$line" "./$desktop_file"
			result=$(desktop-file-validate "./$desktop_file")

			rm "./$desktop_file"
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

# Command line completion
## Check the sections: actions, options_basic, options_remote
## The file_types section is not checked.
## Look for options not included and options erroneously included.

if [ ! -d ./build ]
then
	meson setup build
	ninja -C build
else
	if [ ! -f ./build/src/geeqie ]
	then
		ninja -C build
	fi
fi

geeqie_exe=$(realpath ./build/src/geeqie)

actions_cc=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
actions_help=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
actions_help_cut=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
help_output=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
options_basic_cc=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
options_basic_help=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
options_remote_cc=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")
options_remote_help=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")

options_basic1=$(grep 'options_basic=' ./auto-complete/geeqie)
options_basic2=$(echo "$options_basic1" | cut -c 16-)
options_basic3=$(echo "$options_basic2" | sed "s/\x27//g")
options_basic4=$(echo "$options_basic3" | sed "s/ /\n/g")
echo "$options_basic4" | sort > "$options_basic_cc"

options_remote1=$(grep 'options_remote=' ./auto-complete/geeqie)
options_remote2=$(echo "$options_remote1" | cut -c 17-)
options_remote3=$(echo "$options_remote2" | sed "s/\x27//g")
options_remote4=$(echo "$options_remote3" | sed "s/ /\n/g")
echo "$options_remote4" | sort > "$options_remote_cc"

action_list1=$(grep 'actions=' ./auto-complete/geeqie)
action_list2=$(echo "$action_list1" | cut --delimiter='=' --fields=2)
action_list3=$(echo "$action_list2" | sed "s/\x27//g")
action_list4=$(echo "$action_list3" | sed 's/ /\n/g')
echo "$action_list4" | sort > "$actions_cc"

./scripts/isolate-test.sh xvfb-run --auto-servernum "$geeqie_exe" --help > "$help_output"

awk --lint=fatal --posix --assign options_basic_help="$options_basic_help" --assign options_remote_help="$options_remote_help" '
BEGIN {
valid_found = 0
}

/Valid options/ {valid_found = 1}
/Remote command/ {valid_found = 0}
/--/ && valid_found {
	start = match($0, /--/)
	new=substr($0, start)
	{gsub(/ .*/, "", new)}
	{gsub(/=.*/, "=", new)}
	{gsub(/\[.*/, "", new)}
	print new >> options_basic_help
	}

/--/ && ! valid_found {
	start = match($0, /--/)
	new = substr($0, start)
	{gsub(/ .*/, "", new)}
	{gsub(/=.*/, "=", new)}
	{gsub(/\[.*/, "", new)}
	print new >> options_remote_help
	}

END {
close(options_basic_help)
close(options_remote_help)
}
' "$help_output"

# https://backreference.org/2010/02/10/idiomatic-awk/ is a good reference
awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	exit_status = 1
	print "Bash completions - Basic option missing: " $0
	}

END {
exit exit_status
}
' "$options_basic_cc" "$options_basic_help"

if [ $? = 1 ]
then
	exit_status=1
fi

awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	exit_status = 1
	print "Bash completions - Basic option error: " $0
	}

END {
exit exit_status
}
' "$options_basic_help" "$options_basic_cc"

if [ $? = 1 ]
then
	exit_status=1
fi

awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	exit_status = 1
	print "Bash completions - Remote option missing: " $0
	}

END {
exit exit_status
}
' "$options_remote_cc" "$options_remote_help"

if [ $? = 1 ]
then
	exit_status=1
fi

## @FIXME Differing configuration options are not handled
awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	if (index($0, "lua") == 0)
		{
		exit_status = 1
		print "Bash completions - Remote option error: " $0
		}
	}

END {
exit exit_status
}
' "$options_remote_help" "$options_remote_cc"

if [ $? = 1 ]
then
	exit_status=1
fi

./scripts/isolate-test.sh xvfb-run --auto-servernum "$geeqie_exe" --remote --action-list --quit | cut --delimiter=' ' --fields=1 | sed '/^$/d' > "$actions_help"

awk --lint=fatal --posix --assign actions_help_cut="$actions_help_cut" '
BEGIN {
remote_found = 0
}

/Remote/ {
	remote_found = 1
	next
	}

/.*?/ && remote_found {
	print $0 >> actions_help_cut
	}

END {
close(actions_help_cut)
}
' "$actions_help"

awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	print "Bash completions - Action missing: " $0
	exit_status = 1
	}

END {
exit exit_status
}
' "$actions_cc" "$actions_help_cut"

if [ $? = 1 ]
then
	exit_status=1
fi

awk --lint=fatal --posix '
BEGIN {
exit_status = 0
}

NR == FNR{a[$0]="";next} !($0 in a) {
	print "Bash completions - Action error: " $0
	exit_status = 1
	}

END {
exit exit_status
}
' "$actions_help_cut" "$actions_cc"

if [ $? = 1 ]
then
	exit_status=1
fi

rm --force "$actions_cc"
rm --force "actions_help"
rm --force "actions_help_cut"
rm --force "help_output"
rm --force "options_basic_cc"
rm --force "options_basic_help"
rm --force "options_remote_cc"
rm --force "options_remote_help"

exit "$exit_status"
