#!/usr/bin/awk -f
#
## @file
## @brief Used in the generation of the help file in pdf format.
##
## The sequence is: \n
## xsltproc: .xml to .fo (intermediate file) \n
## fop: .fo to intermediate.pdf (includes ToC at head of file but not in sidebar) \n
## pdftotext: intermediate.pdf to pdf.txt (text of ToC section of .pdf file) \n
## awk: pdf.txt to toc.txt (process pdf.txt ToC to format for pdfoutline) \n
## pdfoutline: intermediate.pdf and toc.txt to help.pdf
##     (combined .pdf with ToC in sidebar)
##
## output_file is an environment variable set in doc/meson.build because
##    meson cannot handle ">" redirection
##
## Output from pdftotext: indentation changes on page iii (reason unknown) \n
## page_offset is the number of pages before Chapter 1. Unlikely to change,
##    but note that (page_offset - 1) is in doc/meson.build.  \n
## Indentation in text is used to determine ToC level \n
## It is assumed that level 2 is the maximum. \n\n
##
## Sample text to process:
## 2. Main Window ........................................ 3 \n
##            File Pane .................................. 3 \n
##                  List view ............................ 3 \n
##
##
## Sample converted text: \n
## 0 11 2. Main Window \n
## 1 11 File Pane \n
## 2 11 List view \n
##

BEGIN {
LINT = "fatal"

level1_offset = 8
level2_offset = 12
page_offset = 8

{print "0 1 The Geeqie User Manual" > output_file}
{print "0 3 Table of Contents" > output_file}
{print "0 " page_offset " List of Tables" > output_file}
}

/Table of Contents/ {next}
/The Geeqie User Manual/ {next}
/   iii$/ {
    level1_offset = 3
    level2_offset = 6
    {next}
    }
/   iv$/ {next}
/   v$/ {next}
/   vi$/ {next}
/   vii$/ {next}
/   viii$/ {next}
/^$/ {next}

{match($0, /^[ ]*/)
if (RLENGTH > level2_offset) {
    level = 2;
    match($0, /[0-9]{1,3}$/)
    page_no=substr($0, RSTART, RLENGTH) + page_offset
    {sub(/^[ ]+/, ""); match ($0, / \.\.\./) }
    {title=substr($0, 1, RSTART - 1)}
    {print level " " page_no " " title > output_file}
    next
    }
}

{match($0, /^[ ]*/)
if (RLENGTH > level1_offset) {
    level = 1;
    match($0, /[0-9]{1,3}$/)
    page_no=substr($0, RSTART, RLENGTH) + page_offset
    {sub(/^[ ]+/, ""); match ($0, / \.\.\./) }
    {title=substr($0, 1, RSTART - 1)}
    {print level " " page_no " " title > output_file}
    next
    }
}

{level = 0;
match($0, /[0-9]{1,3}$/)
page_no=substr($0, RSTART, RLENGTH) + page_offset
{sub(/^[ ]+/, ""); match ($0, / \.\.\./) }
{title=substr($0, 1, RSTART - 1)}
{print level " " page_no " " title > output_file}
next
}

END {
close(output_file)
}
