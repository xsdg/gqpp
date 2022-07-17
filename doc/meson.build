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

guideindex = files('docbook/GuideIndex.xml')
destdir = join_paths(meson.project_build_root(), 'doc', 'html/')
iconsdir = join_paths(meson.project_source_root(), 'doc', 'icons/')

run_command(gnome_doc_tool, 'html', '-o', destdir, '-p', iconsdir, guideindex, check : false)

install_subdir(destdir, install_dir : helpdir, install_tag : 'help')

srcdir = join_paths(meson.project_source_root())
destdir = join_paths(meson.project_build_root(), 'doc', 'html', 'lua-api')

run_command(find_program('create-doxygen-lua-api.sh'), srcdir, destdir, check : false)

install_subdir(destdir, install_dir : htmldir, install_tag : 'html')
