#!/bin/sh
#**********************************************************************
# Copyright (C) 2023 - The Geeqie Team
#
# Author: Colin Clark
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#**********************************************************************

## @file
## @brief Download full and minimal AppImages from the Continuous build release on GitHub.
## Optionally extract the full size AppImage.
##
## $1: -e | --extract Extract full size AppImage
## The user should modify the symbolic links as appropriate.
##
## Downloads will not be made unless the server version is newer than the local file.
##

show_help()
{
printf "Download the latest Geeqie AppImages from the
Continuous Build release on GitHub.

The Continuous Build release is updated each
time the source code is updated.

You should read the contents of this file before
running it and modify it to your requirements.

The default action is to download both the
minimal and full size AppImages to \$HOME/bin.
Symbolic links will be set so that:
\"geeqie\" points to the minimal version
and
\"Geeqie\" points to the full version.

No downloads will be made unless the file on the
server at GitHub is newer than the local file.

The full size AppImage is about 120MB and the
minimal AppImage is about 10MB. Therefore the full
size version will load much slower and will have
a slightly slower run speed.

However the minimal version has limited capabilities
compared to the full size version.

The extract option (-e or --extract) will extract
The contents of the full size image into a sub-directory
of \$HOME/bin, and then set the symbolic link to the
extracted executable. This will take up disk space, but the
extracted executable will run as fast as a packaged release.

When a new file is downloaded the extracted files will
be replaced by the newly downloaded files\n\n"
}

extract=0

while :; do
    case $1 in
        -h|-\?|--help)
            show_help
            exit
            ;;
        -e|--extract)    # AppImage extraction is required
            extract=1
            ;;
        --)              # End of all options.
            shift
            break
            ;;
        -?*)
            printf 'ERROR: Unknown option %s\n' "$1" >&2
            exit
            ;;
        *)
            break
    esac

    shift
done

cd "$HOME/bin/" || exit
architecture=$(arch)

# Download full size AppImage
log_file=$(mktemp "${TMPDIR:-/tmp}/geeqie.XXXXXXXXXX")

wget --no-verbose --show-progress --backups=3 --output-file="$log_file" --timestamping "https://github.com/BestImageViewer/geeqie/releases/download/continuous/Geeqie-latest-$architecture.AppImage";

download_size=$(stat --printf "%s" "$log_file")
rm "$log_file"

# If a new file was downloaded, check if extraction is required
if [ "$download_size" -gt 0 ]
then
	chmod +x "Geeqie-latest-$architecture.AppImage"

	if [ "$extract" -eq 1 ]
	then
		rm -rf "Geeqie-latest-$architecture-AppImage"
		mkdir "Geeqie-latest-$architecture-AppImage"
		cd "Geeqie-latest-$architecture-AppImage" || exit

		printf "Extracting AppImage\n"
		../"Geeqie-latest-$architecture.AppImage" --appimage-extract | cut -c 1-50 | tr '\n' '\r'

		printf "\n"
		cd ..
		ln --symbolic --force "./Geeqie-latest-$architecture-AppImage/squashfs-root/AppRun" Geeqie
	else
		ln --symbolic --force "Geeqie-latest-$architecture.AppImage" Geeqie
	fi
fi

# Download minimal AppImage
wget --quiet --show-progress --backups=3 --timestamping "https://github.com/BestImageViewer/geeqie/releases/download/continuous/Geeqie-minimal-latest-$architecture.AppImage";
chmod +x "Geeqie-minimal-latest-$architecture.AppImage"
ln --symbolic --force "Geeqie-minimal-latest-$architecture.AppImage" geeqie
