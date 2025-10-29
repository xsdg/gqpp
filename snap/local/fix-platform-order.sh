#!/bin/sh
# Reorder LD_LIBRARY_PATH so that any gnome-platform directories come first.

old_IFS=$IFS
IFS=:
front=""
back=""

for s in $LD_LIBRARY_PATH; do
    case "$s" in
        *"/gnome-platform/"*)
            [ -z "$front" ] && front=$s || front="$front:$s"
            ;;
        *)
            [ -z "$back" ] && back=$s || back="$back:$s"
            ;;
    esac
done

IFS=$old_IFS

if [ -n "$front" ]; then
    export LD_LIBRARY_PATH="$front:$back"
else
    export LD_LIBRARY_PATH="$back"
fi

exec "$@"
