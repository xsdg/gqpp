#!/usr/bin/env bash
# shellcheck disable=SC2154
set -euo pipefail

CACHE_DIR="${SNAP_USER_COMMON}/.cache"
CACHE_FILE="${CACHE_DIR}/gdk-pixbuf-loaders.cache"
mkdir -p "${CACHE_DIR}"

# Find the gdk-pixbuf loaders directory (…/gdk-pixbuf-2.0/<ver>/loaders)
discover_module_dir() {
  # Search inside the app snap first, then the gnome platform
  local d
  while IFS= read -r d; do
    [ -d "$d" ] && { echo "$d"; return 0; }
  done < <(find "$SNAP/usr/lib" -type d -path "*/gdk-pixbuf-2.0/*/loaders" -print 2>/dev/null | sort | head -n1)

  while IFS= read -r d; do
    [ -d "$d" ] && { echo "$d"; return 0; }
  done < <(find "$SNAP/gnome-platform/usr/lib" -type d -path "*/gdk-pixbuf-2.0/*/loaders" -print 2>/dev/null | sort | head -n1)

  return 1
}

# Find the gdk-pixbuf-query-loaders binary (it may live under usr/lib/.../gdk-pixbuf-2.0/)
discover_query_tool() {
  # PATH first (in case you staged libgdk-pixbuf2.0-bin)
  if command -v gdk-pixbuf-query-loaders >/dev/null 2>&1; then
    command -v gdk-pixbuf-query-loaders; return 0
  fi
  # Search in both the app snap and the platform
  local p
  while IFS= read -r p; do
    [ -x "$p" ] && { echo "$p"; return 0; }
  done < <(find "$SNAP" "$SNAP/gnome-platform" -type f -executable -name "gdk-pixbuf-query-loaders" -print 2>/dev/null | sort | head -n1)

  return 1
}

MODULEDIR="$(discover_module_dir || true)"
QUERY="$(discover_query_tool || true)"

# Export env for the launched app
[ -n "${MODULEDIR:-}" ] && export GDK_PIXBUF_MODULEDIR="$MODULEDIR"
export GDK_PIXBUF_MODULE_FILE="$CACHE_FILE"

# Rebuild cache if we found both pieces
if [ -n "${QUERY:-}" ] && [ -n "${MODULEDIR:-}" ]; then
  GDK_PIXBUF_MODULEDIR="$MODULEDIR" "$QUERY" > "$CACHE_FILE" 2>/dev/null || true
else
  echo "Note: could not regenerate GDK pixbuf cache (query tool or module dir missing)." >&2
fi

exec "$@"

