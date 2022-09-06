#!/bin/sh

## @file
## @brief Locate strings not marked for translation
##
## Checks all .c files under ./src
##
## The check is not comprehensive - some errors are not detected
## and there are some false hits.
##

for file in src/*.c src/view-file/*.c
do
	for search_text in "label" "menu_item_add" "tooltip" "_button" "_text"
	do
		cat -n "$file" | grep --extended-regexp --ignore-case "$search_text.*\(\"" | grep --invert-match "_(" | grep --invert-match "(\"\")" && printf '%s\n\n' "$file"
	done
done
