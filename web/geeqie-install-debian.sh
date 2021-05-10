#!/bin/bash
version="2021-05-10"
description=$'
Geeqie is an image viewer.
This script will download, compile, and install Geeqie on Debian-based systems.
If run from a folder that already contains the Geeqie sources, the source
code will be updated from the repository.
Dialogs allow the user to install additional features.

Command line options are:
-v --version The version of this file
-h --help Output this text
-c --commit=ID Checkout and compile commit ID
-t --tag=TAG Checkout and compile TAG (e.g. v1.4 or v1.3)
-b --back=N Checkout commit -N (e.g. "-b 1" for last-but-one commit)
-d --debug=yes Compile with debug output
'

# Essential for compiling
essential_array=(
"git"
"build-essential"
"autoconf"
"libglib2.0-0"
"intltool"
"libtool"
"yelp-tools"
)

# Optional for both GTK2 and GTK3
optional_array=(
"LCMS (for color management)"
"liblcms2-dev"
"exiv2 (for exif handling)"
"libgexiv2-dev"
"lua (for --remote commands)"
"liblua5.1-0-dev"
"libffmpegthumbnailer (for mpeg thumbnails)"
"libffmpegthumbnailer-dev"
"libtiff (for tiff support)"
"libtiff-dev"
"libjpeg (for jpeg support)"
"libjpeg-dev"
"librsvg2 (for viewing .svg images)"
"librsvg2-common"
"libwmf (for viewing .wmf images)"
"libwmf0.2-7-gtk"
"exiftran (for image rotation)"
"exiftran"
"imagemagick (for image rotation)"
"imagemagick"
"ufraw (for RAW file handling)"
"ufraw"
"exiv2 command line (for jpeg export)"
"exiv2"
"jpgicc (for jpeg export color correction)"
"liblcms2-utils"
"markdown (for generating README help file)"
"markdown"
"gphoto2 (for tethered photography and camera download plugins)"
"gphoto2"
"libimage-exiftool-perl (for jpeg extraction plugin)"
"libimage-exiftool-perl"
"libheif (for HEIF support)"
"libheif-dev"
"libwebp (for WebP images)"
"libwebp-dev"
"libdjvulibre (for DjVu images)"
"libdjvulibre-dev"
"libopenjp2 (for JP2 images)"
"libopenjp2-7-dev"
"libraw (for CR3 images)"
"libraw-dev"
)

# Optional for GTK3 only
optional_gtk3_array=(
"libchamplain gtk (for GPS maps)"
"libchamplain-gtk-0.12-dev"
"libchamplain (for GPS maps)"
"libchamplain-0.12-dev"
"libpoppler (for pdf file preview)"
"libpoppler-glib-dev"
)

####################################################################
# Get System Info
# Derived from: https://github.com/coto/server-easy-install (GPL)
####################################################################
lowercase()
{
	echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

systemProfile()
{
	OS=`lowercase \`uname\``
	KERNEL=`uname -r`
	MACH=`uname -m`

	if [ "${OS}" == "windowsnt" ]
	then
		OS=windows
	elif [ "${OS}" == "darwin" ]
	then
		OS=mac
	else
		OS=`uname`
		if [ "${OS}" = "SunOS" ]
		then
			OS=Solaris
			ARCH=`uname -p`
			OSSTR="${OS} ${REV}(${ARCH} `uname -v`)"
		elif [ "${OS}" = "AIX" ]
		then
			OSSTR="${OS} `oslevel` (`oslevel -r`)"
		elif [ "${OS}" = "Linux" ]
		then
			if [ -f /etc/redhat-release ]
			then
				DistroBasedOn='RedHat'
				DIST=`cat /etc/redhat-release |sed s/\ release.*//`
				PSUEDONAME=`cat /etc/redhat-release | sed s/.*\(// | sed s/\)//`
				REV=`cat /etc/redhat-release | sed s/.*release\ // | sed s/\ .*//`
			elif [ -f /etc/SuSE-release ]
			then
				DistroBasedOn='SuSe'
				PSUEDONAME=`cat /etc/SuSE-release | tr "\n" ' '| sed s/VERSION.*//`
				REV=`cat /etc/SuSE-release | tr "\n" ' ' | sed s/.*=\ //`
			elif [ -f /etc/mandrake-release ]
			then
				DistroBasedOn='Mandrake'
				PSUEDONAME=`cat /etc/mandrake-release | sed s/.*\(// | sed s/\)//`
				REV=`cat /etc/mandrake-release | sed s/.*release\ // | sed s/\ .*//`
			elif [ -f /etc/debian_version ]
			then
				DistroBasedOn='Debian'
				if [ -f /etc/lsb-release ]
				then
					DIST=`cat /etc/lsb-release | grep '^DISTRIB_ID' | awk -F=  '{ print $2 }'`
					PSUEDONAME=`cat /etc/lsb-release | grep '^DISTRIB_CODENAME' | awk -F=  '{ print $2 }'`
					REV=`cat /etc/lsb-release | grep '^DISTRIB_RELEASE' | awk -F=  '{ print $2 }'`
				fi
			fi
			if [ -f /etc/UnitedLinux-release ]
			then
				DIST="${DIST}[`cat /etc/UnitedLinux-release | tr "\n" ' ' | sed s/VERSION.*//`]"
			fi
			OS=`lowercase $OS`
			DistroBasedOn=`lowercase $DistroBasedOn`
			readonly OS
			readonly DIST
			readonly DistroBasedOn
			readonly PSUEDONAME
			readonly REV
			readonly KERNEL
			readonly MACH
		fi
	fi
}

install_essential()
{
	arraylength=${#essential_array[@]}
	for (( i=0; i<${arraylength}; i=i+1 ));
	do
		package_query ${essential_array[$i]}
		if [ $? != 0 ]
		then
			package_install ${essential_array[$i]}
		fi
	done

	if [[ $1 == "GTK3" ]]
	then
		package_query "libgtk-3-dev"
		if [ $? != 0 ]
		then
			package_install libgtk-3-dev
		fi
	else
		package_query "libgtk2.0-dev"
		if [ $? != 0 ]
		then
			package_install libgtk2.0-dev
		fi
	fi
}

install_options()
{
	if [ -n "$options" ]
	then
		OLDIFS=$IFS
		IFS='|'
		set $options
		while [ $# -gt 0 ];
		do
			package_install $1
			shift
		done
		IFS=$OLDIFS
	fi
}

uninstall()
{
	current_dir=$(basename $PWD)
	if [[ $current_dir == "geeqie" ]]
	then
		sudo --askpass make uninstall
		zenity --title="Uninstall Geeqie" --width=370 --text="WARNING.\nThis will delete folder:\n\n$PWD\n\nand all sub-folders!" --question --ok-label="Cancel" --cancel-label="OK" 2>/dev/null

		if [[ $? == 1 ]]
		then
			cd ..
			sudo --askpass rm -rf geeqie
		fi
	else
		zenity --title="Uninstall Geeqie" --width=370 --text="This is not a geeqie installation folder!\n\n$PWD" --warning 2>/dev/null
	fi

	exit_install
}

package_query()
{
	if [[ $DistroBasedOn == "debian" ]]
	then
		res=$(dpkg-query --show --showformat='${Status}' $1 2>>$install_log)
		if [[ "$res" == "install ok installed"* ]]
		then
			status=0
		else
			status=1
		fi
	fi
	return $status
}

package_install()
{
	if [[ $DistroBasedOn == "debian" ]]
	then
		sudo --askpass apt-get --assume-yes install $@ >>$install_log 2>&1
	fi
}

exit_install()
{
	rm $install_pass_script >/dev/null 2>&1
	#~ rm $install_log >/dev/null 2>&1

	if [[ -p $zen_pipe ]]
	then
		echo "100" > $zen_pipe
		echo "#End" > $zen_pipe
	fi

	zenity --title="$title" --width=370 --text=$'Geeqie is not installed\nLog file: '$install_log --info 2>/dev/null

	rm $zen_pipe >/dev/null 2>&1

	exit 1
}

# Entry point

# If uninstall has been run, maybe the current directory no longer exists
ls $PWD >/dev/null
if [[ $? != 0 ]]
then
	zenity --error --title="Install Geeqie and dependencies" --width=370 --text="Folder $PWD does not exist!" 2>/dev/null

	exit
fi

# Check system type
systemProfile
if [[ $DistroBasedOn != "debian" ]]
then
	zenity --error --title="Install Geeqie and dependencies" --width=370 --text="Unknown operating system:\n
Operating System: $OS
Distribution: $DIST
Psuedoname: $PSUEDONAME
Revision: $REV
DistroBasedOn: $DistroBasedOn
Kernel: $KERNEL
Machine: $MACH" 2>/dev/null

	exit
fi

# Parse the command line
OPTS=$(getopt -o vhc:t:b:d: --long version,help,commit:,tag:,back:,debug: -- "$@")
eval set -- "$OPTS"

while true;
do
	case "$1" in
	-v | --version )
		echo "$version"
		exit
		;;
	-h | --help )
		echo "$description"
		exit
		;;
	-c | --commit )
		COMMIT="$2"
		shift
		shift
		;;
	-t | --tag )
		TAG="$2"
		shift;
		shift
		;;
	-b | --back )
		BACK="$2"
		shift;
		shift
		;;
	-d | --debug )
		DEBUG="$2"
		shift;
		shift
		;;
	* ) break
		;;
	esac
done

# If a Geeqie folder already exists here, warn the user
if [ -d "geeqie" ]
then
	zenity --info --title="Install Geeqie and dependencies" --width=370 --text="This script is for use on Ubuntu and other\nDebian-based installations.\nIt will download, compile, and install Geeqie source\ncode and its dependencies.\n\nA sub-folder named \"geeqie\" will be created in the\nfolder this script is run from, and the source code\nwill be downloaded to that sub-folder.\n\nA sub-folder of that name already exists.\nPlease try another folder." 2>/dev/null

	exit
fi

# If it looks like a Geeqie download folder, assume an update
if [ -d ".git" ] && [ -d "src" ] && [ -f "geeqie.1" ]
then
	mode="update"
else
	# If it looks like something else is already installed here, warn the user
	if [ -d ".git" ] || [ -d "src" ]
	then
		zenity --info --title="Install Geeqie and dependencies" --width=370 --text="This script is for use on Ubuntu and other\nDebian-based installations.\nIt will download, compile, and install Geeqie source\ncode and its dependencies.\n\nIt looks like you are running this script from a folder which already has software installed.\n\nPlease try another folder." 2>/dev/null

		exit
	else
		mode="install"
	fi
fi

# Use GTK3 as default
gk2_installed=FALSE
gtk3_installed=TRUE

if [[ $mode == "install" ]]
then
	message="This script is for use on Ubuntu and other\nDebian-based installations.\nIt will download, compile, and install Geeqie source\ncode and its dependencies.\n\nA sub-folder named \"geeqie\" will be created in the\nfolder this script is run from, and the source code\nwill be downloaded to that sub-folder.\n\nIn this dialog you must select whether to compile\nfor GTK2 or GTK3.\nIf you want to use GPS maps or pdf preview,\nyou must choose GTK3.\nThe GTK2 version has a slightly different\nlook-and-feel compared to the GTK3 version,\nbut otherwise has the same features.\nYou may easily switch between the two after\ninstallation.\n\nIn subsequent dialogs you may choose which\noptional features to install."

	title="Install Geeqie and dependencies"
	install_option=TRUE
else
	message="This script is for use on Ubuntu and other\nDebian-based installations.\nIt will update the Geeqie source code and its\ndependencies, and will compile and install Geeqie.\n\nYou may also switch the installed version from\nGTK2 to GTK3 and vice versa.\n\nIn this dialog you must select whether to compile\nfor GTK2 or GTK3.\nIf you want to use GPS maps or pdf preview,\nyou must choose GTK3.\nThe GTK2 version has a slightly different\nlook-and-feel compared to the GTK3 version,\nbut otherwise has the same features.\n\nIn subsequent dialogs you may choose which\noptional features to install."

	title="Update Geeqie and re-install"
	install_option=FALSE

	# When updating, use previous installation as default
	if [[ -f config.log ]]
	then
		grep gtk-2.0 config.log >/dev/null
		if [[ $? != 0 ]]
		then
			gtk2_installed=FALSE
			gtk3_installed=TRUE
		else
			gtk2_installed=TRUE
			gtk3_installed=FALSE
		fi
	fi
fi

# Ask whether to install GTK2 or GTK3 or uninstall

gtk_version=$(zenity --title="$title" --width=370 --text="$message" --list --radiolist --column "" --column "" "$gtk3_installed" "GTK3 (required for GPS maps and pdf preview)" "$gtk2_installed" "GTK2" FALSE "Uninstall" --cancel-label="Cancel" --ok-label="OK" --hide-header 2>/dev/null)

if [[ $? == 1 ]]
then
	exit
fi

# Environment variable SUDO_ASKPASS cannot be "zenity --password",
# so create a temporary script containing the command
install_pass_script=$(mktemp --tmpdir geeqie_install_XXXXXX.sh)
echo $'#!/bin/bash
zenity --password --title=\"'$title$'\" --width=370 2>/dev/null
if [[ $? > 0 ]]
then
	exit 1
fi
\n' > $install_pass_script
chmod +x $install_pass_script
export SUDO_ASKPASS=$install_pass_script

if [[ $gtk_version == "Uninstall" ]]
then
	uninstall
	exit
fi

# Put the install log in tmp, to avoid writing to PWD during a new install
rm install.log 2>/dev/null
install_log=$(mktemp --tmpdir geeqie_install_XXXXXX.log)

sleep 100 | zenity --title="$title" --text="Checking for installed files" --width=370 --progress --pulsate 2>/dev/null &
zen_pid=$!

# Get the standard options that are not yet installed
arraylength=${#optional_array[@]}
for (( i=0; i<${arraylength}; i=i+2 ));
do
	package_query ${optional_array[$i+1]}
	if [ $? != 0 ]
	then
		if [ -z "$option_string" ]
		then
			option_string="$install_option"$'\n'"${optional_array[$i]}"$'\n'"${optional_array[$i+1]}"
		else
			option_string="$option_string"$'\n'"$install_option"$'\n'"${optional_array[$i]}"$'\n'"${optional_array[$i+1]}"
		fi
	fi
done

# If GTK3 required, get the GTK3 options not yet installed
if [[ "$gtk_version" == "GTK3"* ]]
then
	arraylength=${#optional_gtk3_array[@]}
	for (( i=0; i<${arraylength}; i=i+2 ));
	do
		package_query ${optional_gtk3_array[$i+1]}
		if [ $? != 0 ]
		then
			if [ -z "$option_string" ]
			then
				option_string="$install_option"$'\n'"${optional_gtk3_array[$i]}"$'\n'"${optional_gtk3_array[$i+1]}"
			else
				option_string="$option_string"$'\n'"$install_option"$'\n'"${optional_gtk3_array[$i]}"$'\n'"${optional_gtk3_array[$i+1]}"
			fi
		fi
	done
fi

kill $zen_pid 2>/dev/null

# Ask the user which options to install
if [ -n "$option_string" ]
then
	options=$(echo "$option_string" | zenity --title="$title" --width=400 --height=500 --list --checklist --text 'Select which library files to install:' --column='Select' --column='Library files' --column='Library' --hide-column=3 --print-column=3 2>/dev/null)

	if [[ $? == 1 ]]
	then
		exit_install
	fi
fi

# Start of Zenity progress section
zen_pipe=$(mktemp --dry-run --tmpdir geeqie_install_pipe_XXXXXX)
mkfifo $zen_pipe
(tail  -f $zen_pipe 2>/dev/null) | zenity --progress --title="$title" --width=370 --text="Installing options..." --auto-close --auto-kill --percentage=0 2>/dev/null &

echo "2" > $zen_pipe
echo "#Installing essential libraries..." > $zen_pipe

install_essential $gtk_version

echo "4" > $zen_pipe
echo "#Installing options..." > $zen_pipe

install_options

echo "6" > $zen_pipe
echo "#Installing extra loaders..." > $zen_pipe

echo "10" > $zen_pipe
echo "#Getting new sources from server..." > $zen_pipe

if [[ $mode == "install" ]]
then
	ret=$(git clone git://www.geeqie.org/geeqie.git >>$install_log 2>&1)
else
	git checkout master >>$install_log 2>&1
	if [[ $? != 0 ]]
	then
		git_error=$(tail -n5 $install_log 2>&1)
		zenity --title="$title" --width=370 --height=400 --error --text=$'Git error:\n\n'"$git_error" 2>/dev/null
		exit_install
	fi
	ret=$(git pull >>$install_log 2>&1)
fi

if [[ $? != 0 ]]
then
	git_error=$(tail -n5 $install_log 2>&1)
	zenity --title="$title" --width=370 --height=400 --error --text=$'Git error:\n\n'"$git_error" 2>/dev/null
	exit_install
fi

echo "20" > $zen_pipe
echo "#Cleaning installed version..." > $zen_pipe

if [[ $mode == "install" ]]
then
	cd geeqie
else
	sudo --askpass make uninstall >>$install_log 2>&1
	sudo --askpass make maintainer-clean >>$install_log 2>&1
fi

echo "30" > $zen_pipe
echo "#Checkout required version..." > $zen_pipe

if [[ "$BACK" ]]
then
	ret=$(git checkout master~"$BACK" >>$install_log 2>&1)
	if [[ $? != 0 ]]
	then
		git_error=$(tail -n5 $install_log 2>&1)
		zenity --title="$title" --width=370 --height=400 --error --text=$'Git error:\n\n'"$git_error" 2>/dev/null
		exit_install
	fi
elif [[ "$COMMIT" ]]
then
	ret=$(git checkout "$COMMIT" >>$install_log 2>&1)
	if [[ $? != 0 ]]
	then
		git_error=$(tail -n5 $install_log 2>&1)
		zenity --title="$title" --width=370 --height=400 --error --text=$'Git error:\n\n'"$git_error" 2>/dev/null
		exit_install
	fi
elif [[ "$TAG" ]]
then
	ret=$(git checkout "$TAG" >>$install_log 2>&1)
	if [[ $? != 0 ]]
	then
	echo "error"
		git_error=$(tail -n5 $install_log 2>&1)
		zenity --title="$title" --width=370 --height=400 --error --text=$'Git error:\n\n'"$git_error" 2>/dev/null
		exit_install
		exit
	fi
fi
if [[ "$DEBUG" == "yes" ]]
then
	debug_opt=""
else
	debug_opt="--disable-debug-log"
fi

echo "40" > $zen_pipe
echo "#Creating configuration files..." > $zen_pipe

if [[ $gtk_version == "GTK3"* ]]
then
	./autogen.sh "$debug_opt" >>$install_log 2>&1
else
	./autogen.sh "$debug_opt" --disable-gtk3 >>$install_log 2>&1
fi

echo "60" > $zen_pipe
echo "#Compiling..." > $zen_pipe

export CFLAGS=$CFLAGS" -Wno-deprecated-declarations"
export CXXFLAGS=$CXXFLAGS" -Wno-deprecated-declarations"
make -j >>$install_log 2>&1

if [[ $? != 0 ]]
then
	zenity --title="$title" --width=370 --height=400 --error --text=$'Compile error' 2>/dev/null
	exit_install
	exit
fi

echo "90 " > $zen_pipe
echo "#Installing Geeqie..." > $zen_pipe

sudo --askpass make install >>$install_log 2>&1

rm $install_pass_script
mv -f $install_log install.log;

echo "100 " > $zen_pipe
rm $zen_pipe

(for i in $(seq 0 4 100); do echo "$i"; sleep 0.1; done) | zenity --progress --title="$title" --width=370 --text="Geeqie installation complete...\n$zonedetect_message" --auto-close --percentage=0 2>/dev/null

exit
