#!/bin/bash

# Script to create README.html file,

[ ! -e "README.md" ] && exit 1
[ ! -x "$(command -v markdown)" ] && exit 1

[ -e README.html ] && mv -f README.html README.html.bak

markdown README.md > README.html

exit 0
