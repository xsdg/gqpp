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
graphicsdir = join_paths(meson.current_source_dir(), 'graphics/')
guideindex_ln = join_paths(destdir, 'index.html')

option = get_option('yelp-build')
if not option.disabled()
    gnome_doc_tool = find_program('yelp-build', required : false)
    if gnome_doc_tool.found()
        help_html = custom_target(
            'help-html',
            input : guideindex_xml,
            output : ['html'],
            command : [gnome_doc_tool, 'html', '@INPUT@', '-o', destdir, '-p', graphicsdir],
            install : true,
            install_dir : helpdir)
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

# Create the help file in .pdf format with ToC in the pdf viewer sidebar
option = get_option('help_pdf')
if not option.disabled()
    xmldir =  join_paths(meson.current_source_dir(), 'docbook/')

    awk_exe = find_program('awk', required : false)
    fop_exe = find_program('fop', required : false)
    pdfoutline_exe = find_program('pdfoutline', required : false)
    pdftotext_exe = find_program('pdftotext', required : false)
    xsltproc_exe = find_program('xsltproc', required : false)

    if awk_exe.found() and fop_exe.found() and pdfoutline_exe.found() and pdftotext_exe.found() and xsltproc_exe.found() and fs.is_file('/usr/share/xml/docbook/stylesheet/docbook-xsl/fo/docbook.xsl')

        intermediate_fo = custom_target(
            'create .fo file',
            input : guideindex_xml,
            output : 'output.fo',
            command : [xsltproc_exe, '--path', xmldir, '--stringparam', 'img.src.path',  graphicsdir, '--output', '@OUTPUT@', '--stringparam', 'paper.type', 'A4', '--xinclude', '/usr/share/xml/docbook/stylesheet/docbook-xsl/fo/docbook.xsl', '@INPUT@'],
            )

        # Create intermediate .pdf file - no ToC in sidebar',
        help_intermediate_pdf = custom_target(
            'create intermediate pdf',
            input : intermediate_fo,
            output : 'help_inter.pdf',
            command : ['sh', '-c', '"$1" -fo "$2" -pdf "$3" 2>/dev/null', 'ignored', fop_exe, '@INPUT@', '@OUTPUT@'],
            )

        # Create ToC in text format from intermediate .pdf file
        # -f (first page) and -l (last page) to process unlikely to change
        pdf_as_text = custom_target(
            'create toc as text file',
            input : help_intermediate_pdf,
            output : 'pdf.txt',
            command : [pdftotext_exe, '-f', '3', '-l', '7', '-nopgbrk', '-layout','@INPUT@', '@OUTPUT@'],
            )

        # Convert the text ToC to the format needed by pdfoutline
        toc_as_text = custom_target(
            'create toc layout as text file',
            input : ['toc.awk', pdf_as_text],
            output : 'toc.txt',
            command : [awk_exe, '-v', 'output_file=@OUTPUT@', '-f', '@INPUT0@', '@INPUT1@'],
            )

        # Combine original .pdf with sidebar ToC
        help_pdf = custom_target(
            'create help pdf',
            input : [help_intermediate_pdf, toc_as_text],
            output : 'help.pdf',
            command : [pdfoutline_exe, '@INPUT0@', '@INPUT1@', '@OUTPUT@'],
            install : true,
            install_dir : helpdir,
            )

        summary({'help_pdf' : ['Help file in pdf created:', true]}, section : 'Documentation', bool_yn : true)
    else
        summary({'help-pdf' : ['Required files not found - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
    endif
else
    summary({'help-pdf' : ['disabled - Help file in pdf created:', false]}, section : 'Documentation', bool_yn : true)
endif
