#!/bin/sh

#**********************************************************************
# Copyright (C) 2022 - The Geeqie Team
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
## @brief Compile linuxdeploy for use on arm
##  
## There is no AppImage for linuxdeploy on arm. It must be compiled
## from sources.
##
## This is just a hack until AppImages are available on github.
##

set -x

arch=$(uname -m)
if [ "$arch" != "aarch64" ]
then
	exit
fi

sudo apt install libfuse2
sudo apt install libgirepository1.0-dev

rm -rf /tmp/linuxdeploy-*

cd "$HOME"
if [ ! -d bin ]
then
	mkdir bin
fi

cd "$HOME"/bin

if [ ! -f appimagetool-aarch64.AppImage ]
then
	wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-aarch64.AppImage
	chmod +x appimagetool-aarch64.AppImage
fi

if [ ! -f linuxdeploy-plugin.sh ]
then
	wget https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh
	chmod +x linuxdeploy-plugin-gtk.sh
fi

if [ ! -d linuxdeploy-download ]
then
	git clone --recursive https://github.com/linuxdeploy/linuxdeploy.git
	mv linuxdeploy linuxdeploy-download
fi

cd linuxdeploy-download

# Use arm architecture
sed -i 's/"x86_64"/"aarch64"/g' ./ci/build.sh
sed -i 's/"x86_64"/"aarch64"/g' ./ci/build-static-binutils.sh
sed -i 's/"x86_64"/"aarch64"/g' ./ci/build-static-patchelf.sh

# Always use /tmp
sed -i 's/TEMP_BASE=\/dev\/shm/TEMP_BASE=\/tmp/' ./ci/build.sh

# Do not erase working directory
sed -i 's/trap cleanup EXIT//' ./ci/build.sh

# Fix possible error in current (2022-12-13) version
sed -i 's/\/\/ system headers/#include <array>/' ./src/subprocess/subprocess.cpp
sed -i 's/\/\/ system headers/#include <array>/' ./src/plugin/plugin_process_handler.cpp

# No downloads
sed -i 's/wget/\#/g' ./ci/build.sh

# No exit on fail
sed -i 's/set\ \-e/\#/' .ci/build.sh

cp src/core/copyright/copyright.h src/core

cd ..

ARCH=aarch64 bash linuxdeploy-download/ci/build.sh

find /tmp/linuxdeploy-build* -type f -name linuxdeploy -exec cp {} "$HOME"/bin/ \;
find /tmp/linuxdeploy-build* -type f -name patchelf -exec cp {} "$HOME"/bin \;

if [ ! -d linuxdeploy-plugin-appimage ]
then
	git clone --recursive https://github.com/linuxdeploy/linuxdeploy-plugin-appimage.git
fi

cd linuxdeploy-plugin-appimage

# Always use /tmp
sed -i 's/TEMP_BASE=\/dev\/shm/TEMP_BASE=\/tmp/' ./ci/build-appimage.sh

# Do not erase working directory
sed -i 's/trap cleanup EXIT//' ./ci/build-appimage.sh

# Use arm architecture
sed -i 's/"x86_64"/"aarch64"/g' ./ci/build-appimage.sh

# No downloads
sed -i 's/wget/\#/g' ./ci/build-appimage.sh

# No exit on fail
sed -i 's/set\ \-e/\#/' ./ci/build-appimage.sh

cd ..

ARCH=aarch64 bash linuxdeploy-plugin-appimage/ci/build-appimage.sh


