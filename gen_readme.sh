#!/bin/sh

## @file
## @brief Convert README.md to README.html
##
## Create README.html file,
##

srcdir="$1"
builddir="$2"

if [ ! -e "$srcdir/README.md" ]
then
	printf '%s\n' "ERROR: README.md not found"
	exit 1
fi

if ! command -v pandoc > /dev/null 2>&1
then
	printf '%s\n' "ERROR: File pandoc not installed"
	exit 1
fi

pandoc "$srcdir/README.md" > "$builddir/README.html"

exit 0
