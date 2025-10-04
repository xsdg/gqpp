#!/usr/bin/env bash
set -euo pipefail

# Add any staged BLAS/LAPACK subdirs to LD_LIBRARY_PATH dynamically
for d in "$SNAP/usr/lib"/*/blas "$SNAP/usr/lib"/*/lapack; do
  if [ -d "$d" ]; then
    if [ -n "${LD_LIBRARY_PATH-}" ]; then
      LD_LIBRARY_PATH="$d:$LD_LIBRARY_PATH"
    else
      LD_LIBRARY_PATH="$d"
    fi
  fi
done
export LD_LIBRARY_PATH

exec "$@"
