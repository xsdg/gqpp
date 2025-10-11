#!/bin/sh
# shellcheck disable=SC2154  # SNAP is set by snapd
case "$SNAP_ARCH" in
  amd64)  MA=x86_64-linux-gnu ;;
  arm64)  MA=aarch64-linux-gnu ;;
  armhf)  MA=arm-linux-gnueabihf ;;
  *)      MA="$SNAP_ARCH" ;;
esac

# shellcheck disable=SC2154  # SNAP is set by snapd
export MAGICK_CONFIGURE_PATH="\
$SNAP/etc/ImageMagick-6:\
$SNAP/usr/share/ImageMagick-6:\
$SNAP/usr/lib/$MA/ImageMagick-6/config-Q16:\
$SNAP/usr/lib/$MA/ImageMagick-6.9.12/config-Q16"

# shellcheck disable=SC2154  # SNAP is set by snapd
export MAGICK_CODER_MODULE_PATH="\
$SNAP/usr/lib/$MA/ImageMagick-6/modules-Q16/coders:\
$SNAP/usr/lib/$MA/ImageMagick-6.9.12/modules-Q16/coders"

exec "$@"
