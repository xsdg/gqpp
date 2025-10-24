#!/bin/sh
export DISABLE_WAYLAND=1
export GDK_BACKEND=x11
export CLUTTER_BACKEND=x11
exec "$@"
