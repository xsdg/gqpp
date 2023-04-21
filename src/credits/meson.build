# This file is a part of Geeqie project (https://www.geeqie.org/).
# Copyright (C) 2008 - 2022 The Geeqie Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#

if running_from_git
    run_command(find_program('authors.sh'),  meson.current_build_dir(), 'running_from_git', check : false)
else
    run_command(find_program('authors.sh'),  meson.current_build_dir(), '', check : false)
endif

run_command(find_program('translators.sh'),  meson.source_root(), meson.current_build_dir(), check : false)

message(meson.current_build_dir())
source_string = '--sourcedir=' + meson.current_build_dir()

target_string = '--target=' + meson.current_build_dir() + '/credits.h'
credits_resources_h = custom_target('credits_resources_h',
                input : 'credits.gresource.xml',
                output : 'credits.h',
                command : [glib_compile_resources, '--generate-header', source_string,target_string, '@INPUT@'])

target_string = '--target=' + meson.current_build_dir() + '/credits.c'
credits_resources_c = custom_target('credits_resources_c',
                input : 'credits.gresource.xml',
                output : 'credits.c',
                command : [glib_compile_resources, '--generate-source', target_string, source_string, '@INPUT@'])

project_sources += credits_resources_h
project_sources += credits_resources_c
