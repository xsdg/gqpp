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

subdir('html')

guideindex_xml = files('docbook/GuideIndex.xml')
destdir = join_paths(meson.current_build_dir(), 'html/')
iconsdir = join_paths(meson.current_source_dir(), 'icons/')
guideindex_ln = join_paths(destdir, 'index.html')

option = get_option('yelp-build')
if not option.disabled()
    gnome_doc_tool = find_program('yelp-build', required : false)
    if gnome_doc_tool.found()
        run_command(gnome_doc_tool, 'html', '-o', destdir, '-p', iconsdir, guideindex_xml, check : false)
        run_command(find_program('ln'), '-s', '-f', 'GuideIndex.html', guideindex_ln, check : false)
        summary({'help' : ['Help files created:', true]}, section : 'Documentation', bool_yn : true)
    else
        summary({'help' : ['yelp-build not found - Help files created:', false]}, section : 'Documentation', bool_yn : true)
    endif
else
    summary({'help' : ['disabled - Help files created:', false]}, section : 'Documentation', bool_yn : true)
endif

if running_from_git
    option = get_option('doxygen')
    if not option.disabled()
        doxygen = find_program('doxygen', required : false)
        if doxygen.found()
            srcdir = join_paths(meson.project_source_root())
            destdir = join_paths(meson.project_build_root(), 'doc', 'html', 'lua-api')
            run_command(find_program('create-doxygen-lua-api.sh'), srcdir, destdir, check : false)
            summary({'lua-api' : ['lua-api help file created:', true]}, section : 'Documentation', bool_yn : true)
        else
            summary({'lua-api' : ['doxygen not found - lua-api help file created:', false]}, section : 'Documentation', bool_yn : true)
        endif
    else
        summary({'lua-api' : ['disabled - lua-api help file created:', false]}, section : 'Documentation', bool_yn : true)
    endif
endif


option = get_option('help_pdf')
if not option.disabled()
    xmldir =  join_paths(meson.current_source_dir(), 'docbook/')

    xsltproc_exe = find_program('xsltproc', required : false)
    if xsltproc_exe.found()
        fop_exe = find_program('fop', required : false)
        if fop_exe.found()
            if fs.is_file('/usr/share/xml/docbook/stylesheet/docbook-xsl/fo/docbook.xsl')
                intermediate_fo = custom_target(
                    'create fo file',
                    input : guideindex_xml,
                    output : 'output.fo',
                    command : [xsltproc_exe, '--path', xmldir, '--stringparam', 'img.src.path',  iconsdir, '--output', '@OUTPUT@', '--stringparam', 'paper.type', 'A4', '--xinclude', '/usr/share/xml/docbook/stylesheet/docbook-xsl/fo/docbook.xsl', '@INPUT@'],
                    )

                help_pdf = custom_target(
                    'create pdf file',
                    input : intermediate_fo,
                    output : 'help.pdf',
                    command : [fop_exe, '-fo', '@INPUT@', '-pdf', '@OUTPUT@'],
                    install : true,
                    install_dir : helpdir,
                    )
                summary({'help_pdf' : ['Help file in pdf created:', true]}, section : 'Documentation', bool_yn : true)
            else
                summary({'help_pdf' : ['docbook.xsl not found - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
            endif
        else
            summary({'help_pdf' : ['fop not found - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
        endif
    else
        summary({'help_pdf' : ['xsltproc not found - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
    endif
else
    summary({'help-pdf' : ['disabled - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
endif
