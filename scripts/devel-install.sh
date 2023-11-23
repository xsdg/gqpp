#!/bin/sh

#/*
# * Copyright (C) 2023 The Geeqie Team
# *
# * Author: Colin Clark
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License along
# * with this program; if not, write to the Free Software Foundation, Inc.,
# * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# */

## @file
## @brief Install additional files for Geeqie development purposes
##

if ! { [ -d ".git" ] && [ -d "src" ] && [ -f "geeqie.1" ] && [ -f "doxygen.conf" ]; }
then
    printf "This is not the project root directory\n"
    exit 1
fi

if ! zenity --title="Install files for Geeqie development" --question --text "This script will install:\n
<tt>
curl                # for this script
default-jre         # for doxygen diagrams
libdw-dev           # for devel=enabled
libdwarf-dev        # for devel=enabled
libunwind-dev       # for devel=enabled
shellcheck          # for meson tests
texlive-font-utils  # for doxygen diagrams
xvfb                # for meson tests
</tt>

The following will be downloaded to $HOME/bin/ and made executable:\n
<tt>
https://github.com/plantuml/plantuml/releases/download/<i>latest</i>/plantuml-<i>latest</i>.jar  # for doxygen diagrams
https://raw.githubusercontent.com/Anvil/bash-doxygen/master/doxygen-bash.sed       # for documenting script files\n
</tt>

Continue?" --width=300
then
    exit 0
fi

if ! mkdir -p "$HOME"/bin
then
    printf "Cannot create %s\n" "$HOME"/bin
    exit 1
fi

sudo apt install curl
sudo apt install default-jre
sudo apt install libdw-dev
sudo apt install libdwarf-dev
sudo apt install libunwind-dev
sudo apt install shellcheck
sudo apt install texlive-font-utils
sudo apt install xvfb

cd "$HOME"/bin || exit 1

if ! [ -f doxygen-bash.sed ]
then
    wget https://raw.githubusercontent.com/Anvil/bash-doxygen/master/doxygen-bash.sed
    chmod +x doxygen-bash.sed
fi

latest_plantuml=$(curl --silent -qI https://github.com/plantuml/plantuml/releases/latest | awk -F '/' '/^location/ {print  substr($NF, 1, length($NF)-1)}')

if ! [ -f "plantuml-${latest_plantuml#v}.jar" ]
then
    wget "https://github.com/plantuml/plantuml/releases/download/$latest_plantuml/plantuml-${latest_plantuml#v}.jar"
    ln --symbolic --force "plantuml-${latest_plantuml#v}.jar" plantuml.jar
    chmod +x plantuml.jar
fi


