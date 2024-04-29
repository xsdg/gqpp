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
## @brief Locate strings not marked for translation
##
## The check is not comprehensive - the exclusions make this
## of limited value.
##
## @FIXME Strings starting with a space, or a lower-case alpha
## or if there is more than one string per line, are not
## checked for.
##
## The regex search is for a character sequence: \n
## double quotes \n
## upper-case alpha character \n
## alpha character or space \n
## printable character \n
## previous character type repeated one or more times \n
## double quotes
##
## The above sequence preceded by "_(" will not be a hit.
##
## $1 file to process
##

omit_text_array="
 msg
#define
#include
&lt
@brief
@param
COPYRIGHT
ColorSpace
DEBUG
Damien
ERR
EXIF
Error
Exif\.
FIXME
ImageLoaderType
LUA_
MonoSpace
N_(
NikonTag
Pause
PixbufRenderer
PluginsMenu
READ_
Separator
WRITE_
Wrap
\"Desktop\"
\"Layout\"
\"OK\"
\"Xmp\.
\.html
\/\*
\/\/
\{\"
_attribute
action_group
courier
exif_get_item
filter_add_if_missing
font_name
g_ascii_strcasecmp
g_build_filename
g_critical
g_key_file_get_
g_message
g_object
g_signal
g_str_has_suffix
g_strstr_len
g_themed_icon_new
g_warning
getenv
gtk_action_group_get_action
gtk_container_child_get
gtk_widget_add_accelerator
layout_actions
layout_toolbar_add
luaL_
lua_
memcmp
mouse_button_
options->
osd_template_insert
pango_attr
path_to_utf8
primaries_name
print_term
printf
return g_strdup
runcmd
setenv
signal_
signals_
strcmp
strncmp
trc_name
website-label
write_char_option

##cellrendericon.cc
\"Background color as a GdkRGBA\"\,
\"Background color\"\,
\"Background set\"\,
\"Draw focus indicator\"\,
\"Fixed height\"\,
\"Fixed width\"\,
\"Focus\"\,
\"Foreground color as a GdkRGBA\"\,
\"Foreground color\"\,
\"Foreground set\"\,
\"GQvCellRendererIcon\"\,
\"Height of icon excluding text\"\,
\"Marks bit array\"\,
\"Marks\"\,
\"Number of marks\"\,
\"Pixbuf Object\"\,
\"Show marks\"\,
\"Show text\"\,
\"Text to render\"\,
\"Text\"\,
\"The pixbuf to render\"\,
\"Toggled mark\"\,
\"Whether the marks are displayed\"\,
\"Whether the text is displayed\"\,
\"Whether this tag affects the background color\"\,
\"Whether this tag affects the foreground color\"\,
\"Width of cell\"\,

##pixbuf-renderer.cc
\"Delay image update\"\,
\"Display cache size MiB\"\,
\"Expand image in autozoom.\"\,
\"Fit window to image size\"\,
\"Image actively loading\"\,
\"Image rendering complete\"\,
\"Limit size of image when autofitting\"\,
\"Limit size of parent window\"\,
\"New image scroll reset\"\,
\"Number of tiles to retain in memory at any one time.\"\,
\"Size increase limit of image when autofitting\"\,
\"Size limit of image when autofitting\"\,
\"Size limit of parent window\"\,
\"Tile cache count\"\,
\"Zoom maximum\"\,
\"Zoom minimum\"\,
\"Zoom quality\"\,

##print.cc
G_CALLBACK(print_set_font_cb)\, const_cast<char \*>(\"Image text font\"));
G_CALLBACK(print_set_font_cb)\, const_cast<char \*>(\"Page text font\"));

##remote.cc
render_intent = g_strdup(\"Absolute Colorimetric\");
render_intent = g_strdup(\"Absolute Colorimetric\");
render_intent = g_strdup(\"Perceptual\");
render_intent = g_strdup(\"Relative Colorimetric\");
render_intent = g_strdup(\"Saturation\");
"

exclude_files_array="
exif.cc
format-canon.cc
format-fuji.cc
format-nikon.cc
format-olympus.cc
keymap-template.cc
"

filename_printed="no"

omit="FILE_OK"
while read -r omit_file
do
	if [ -n "$omit_file" ]
	then
		if echo "$1" | grep --quiet "$omit_file"
		then
			omit="omit"
		fi
	fi
done << EOF
$exclude_files_array
EOF

if [ "$omit" = "FILE_OK" ]
then
	while read -r infile_line
	do
		if [ -n "$infile_line" ]
		then
			omit="LINE_NOT_OK"
			while read -r omit_text
			do
				if [ -n "$omit_text" ]
				then
					if echo "$infile_line" | grep --quiet "$omit_text"
					then
						omit="omit"
					fi
				fi
			done << EOF
$omit_text_array
EOF
			if [ "$omit" = "LINE_NOT_OK" ]
			then
				if [ "$filename_printed" = "no" ]
				then
					printf "\nfile: %s\n" "$1"
					filename_printed="yes"
				fi

				no_tabs=$(echo "$infile_line" | tr -s '\t')
				printf "line: %s\n" "$no_tabs"
			fi
		fi
	done << EOF
$(cat --number "$1" | grep --perl-regexp '(?<!_\()"[[:upper:]]([[:lower:]]|[[:space:]])[[:print:]]+"')
EOF
fi

if [ "$filename_printed" = "yes" ]
then
	exit 1
else
	exit 0
fi
