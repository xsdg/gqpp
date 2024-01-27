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

subdir('camera-import')
subdir('export-jpeg')
subdir('geocode-parameters')
subdir('image-crop')
subdir('lens')
subdir('open-with')
subdir('random-image')
subdir('resize-image')
subdir('rotate')
subdir('symlink')
subdir('tethered-photography')
subdir('video-player')

i18n.merge_file(
    input : 'org.geeqie.template.desktop.in',
    output : 'org.geeqie.template.desktop',
    type : 'desktop',
    po_dir : podir,
    install : true,
    install_dir : appdir)

desktop_sources += files('org.geeqie.template.desktop.in')
