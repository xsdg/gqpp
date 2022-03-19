#!/bin/sh

## @file
## @brief Convert README.md to README.html
##
## Ceate README.html file,
##

if [ ! -e "README.md" ]
then
	printf '%s\n' "ERROR: README.md not found"
	exit 1
fi

if ! command -v pandoc > /dev/null 2>&1
then
	printf '%s\n' "ERROR: File pandoc not installed"
	exit 1
fi

[ -e README.html ] && mv -f README.html README.html.bak

pandoc README.md > README.html

exit 0
