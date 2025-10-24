#!/bin/sh
unset DISABLE_WAYLAND
export GDK_BACKEND=wayland
export CLUTTER_BACKEND=wayland
export GQ_DISABLE_CLUTTER=yes
exec "$@"
