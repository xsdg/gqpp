#!/bin/sh
# Prefer Wayland if available & supported; otherwise fall back to X11.
unset DISABLE_WAYLAND
if [ -n "$WAYLAND_DISPLAY" ]
then
  export GDK_BACKEND=wayland
  export CLUTTER_BACKEND=wayland
  export GQ_DISABLE_CLUTTER=yes
else
  export GDK_BACKEND=x11
  export CLUTTER_BACKEND=x11
fi
exec "$@"
