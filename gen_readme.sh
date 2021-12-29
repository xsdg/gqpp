#!/bin/bash

## @file
## @brief Convert README.md to README.html
##
## Script to create README.html file,
##

if [ ! -e "README.md" ]
then
	echo "ERROR: README.md not found"
	exit 1
fi

if [ ! -x "$(command -v pandoc)" ]
then
	echo "ERROR: File pandoc not installed"
	exit 1
fi

[ -e README.html ] && mv -f README.html README.html.bak

pandoc README.md > README.html

exit 0
