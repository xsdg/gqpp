# This file is a part of Geeqie project (https://www.geeqie.org/).
# Copyright (C) 2008 - 2023 The Geeqie Team
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

target_string = '--target=' + meson.current_build_dir() + '/icons.h'
icons_resources_h = custom_target('icons_resources_h',
                input : 'icons.gresource.xml',
                output : 'icons.h',
                command : [glib_compile_resources, '--generate-header', target_string, '--sourcedir=@CURRENT_SOURCE_DIR@', '@INPUT@'])

target_string = '--target=' + meson.current_build_dir() + '/icons.c'
icons_resources_c = custom_target('icons_resources_c',
                input : 'icons.gresource.xml',
                output : 'icons.c',
                command : [glib_compile_resources, '--generate-source', target_string, '--sourcedir=@CURRENT_SOURCE_DIR@', '@INPUT@'])

project_sources += icons_resources_h
project_sources += icons_resources_c

subdir('svg')
