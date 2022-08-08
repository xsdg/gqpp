#!/bin/sh

## @file
## @brief Generate a patch to update POTFILES
##
## Use like this: ./regen_potfiles.sh | patch -p0
##

TMP=POTFILES.$$
((find ../src/ -type f -name '*.c' ; find ../ -type f -name '*.desktop.in' ; find ../ -type f -name '*.appdata.xml.in') | while read f; do
	(echo $f | sed 's#^../##')
done) | sort > $TMP
diff -u POTFILES $TMP
rm -f $TMP
