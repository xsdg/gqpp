#!/bin/bash

## @file
## @brief Create a new release
##
## new-release.sh [option]...\n
## Where:\n
## -v \<a.b\> is a major.minor version number\n
## -s \<c\> is the start hash number for a new major.minor release - if omitted, HEAD is used\n
## -p \<d\> is the patch version if a major.minor.patch release is being created\n
## -r Push the release to the repo. If omitted a test run is made (run from a temp. clone folder)\n
## -h Print Help
##
## Will create a new release off the master branch, or will create a new
## patch version off an existing major.minor release branch.
##
## It is expected that the first line of NEWS is in the form "Geeqie \<a.b[.d]\>
##

if [ ! -d .git ]
then
	echo "Directory .git does not exist"
	exit 1
fi

if ! zenity --title="NEW RELEASE" --question --text "Edit the following files before running\n this  script:\n\nNEWS\norg.geeqie.Geeqie.appdata.xml.in\n\nContinue?" --width=300
then
	exit 0
fi

version=
start=
patch=
push=
while getopts "v:s:p:hr" option; do
	case $option in
	h)
		echo -e "-v <a.b> release major.minor e.g 1.9\n-s <c> start hash e.g. 728172681 (optional)\n-p <d> release patch version e.g. 2 for 1.6.2\n-r push to repo.\n-h help"
		exit 0
		;;
	v) version="$OPTARG" ;;
	s) start="$OPTARG" ;;
	p) patch="$OPTARG" ;;
	r) push=true ;;
	*) exit 1 ;;
	esac
done

if [ "$start" ] && [ "$patch" ]
then
	echo "Cannot have start-hash and patch number together"
	exit 1
fi

if [ "$(echo "$version" | awk -F"." '{print NF-1}')" -ne 1 ]
then
	echo "Version major.minor $version is not valid"
	exit 1
fi

if [ "$start" ]
then
	if ! git branch master --contains "$start" > /dev/null 2>&1
	then
		echo "Start hash is not in master branch"
		exit 1
	fi
fi

if [ "$patch" ]
then
	if ! git rev-parse "v$version" > /dev/null 2>&1
	then
		echo "Version $version does not exist"
		exit 1
	fi

	if [[ ! $patch =~ ^-?[0-9]+$ ]]
	then
		echo "Patch $patch is not an integer"
		exit 1
	fi
else
	if git rev-parse "v$version" > /dev/null 2>&1
	then
		echo "Version $version already exists"
		exit 1
	fi
fi

if [ ! "$patch" ]
then
	revision="$version"
else
	revision="$version.$patch"
fi

if [ ! "$patch" ]
then
	if [ -z "$start" ]
	then
		git checkout -b stable/"$version"
	else
		git checkout -b stable/"$version" "$start"
	fi

	if [ "$push" ]
	then
		git push git@geeqie.org:geeqie stable/"$version"
	fi
else
	git checkout stable/"$version"
fi

sudo make maintainer-clean
./autogen.sh
make -j
./scripts/generate-man-page.sh

git add NEWS
git add org.geeqie.Geeqie.appdata.xml.in
git add geeqie.1
git add doc/docbook/CommandLineOptions.xml
git commit --message="Preparing for release v$revision"

if [ "$push" ]
then
	git push git@geeqie.org:geeqie
fi

git tag --sign "v$revision" --message="Release v$revision"

if [ "$push" ]
then
	git push git@geeqie.org:geeqie "v$revision"
fi

sudo make maintainer-clean
./gen_changelog.sh

rm -rf /tmp/geeqie-"$revision".tar.xz
rm -rf /tmp/geeqie-"$revision".tar.xz.asc

# shellcheck disable=SC2140
tar --create --xz --file=/tmp/geeqie-"$revision".tar.xz --exclude=".git" --exclude="configure" --exclude="web" --transform s/"\bgeeqie\b"/"geeqie-$revision"/ ../geeqie
gpg --armor --detach-sign --output /tmp/geeqie-"$revision".tar.xz.asc  /tmp/geeqie-"$revision".tar.xz

git checkout master

git checkout stable/"$version" NEWS
git checkout stable/"$version" geeqie.1
git checkout stable/"$version" doc/docbook/CommandLineOptions.xml
git checkout stable/"$version" org.geeqie.Geeqie.appdata.xml.in

git add NEWS
git add org.geeqie.Geeqie.appdata.xml.in
git add geeqie.1
git add doc/docbook/CommandLineOptions.xml
git commit --message="Release v$revision files"

if [ "$push" ]
then
	git push git@geeqie.org:geeqie
fi

zenity --info --window-icon="info" --text="Upload files:\n\n/tmp/geeqie-$revision.tar.xz\n/tmp/geeqie-$revision.tar.xz.asc\n\nto https://github.com/BestImageViewer/geeqie/releases" --width=400
