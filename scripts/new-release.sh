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

error_handler ()
{
	echo "Error on line: $1"
	exit 1
}
trap 'error_handler ${LINENO}' ERR


version=
start=
patch=
push=
while getopts "v:s:p:hr" option
do
	case $option in
		h)
			printf '%s\n%s\n%s\n%s\n%s\n' "-v <a.b> release major.minor e.g 1.9" "-s <c> start hash e.g. 728172681 (optional)" "-p <d> release patch version e.g. 2 for 1.6.2" "-r push to repo." "-h help"
			exit 0
			;;
		v) version="$OPTARG" ;;
		s) start="$OPTARG" ;;
		p) patch="$OPTARG" ;;
		r) push=true ;;
		*) exit 1 ;;
	esac
done

if [ ! -d .git ]
then
	printf '%s\n' "Directory .git does not exist"
	exit 1
fi

if ! zenity --title="NEW RELEASE" --question --text "Edit the following files before running\n this  script:\n\nNEWS\norg.geeqie.Geeqie.appdata.xml.in\n\nContinue?" --width=300
then
	exit 0
fi

if [ -n "$start" ] && [ -n "$patch" ]
then
	printf '%s\n' "Cannot have start-hash and patch number together"
	exit 1
fi

if [ "$(printf '%s\n' "$version" | awk -F"." '{print NF-1}')" -ne 1 ]
then
	printf '%s\n' "Version major.minor $version is not valid"
	exit 1
fi

if [ -n "$start" ]
then
	if ! git branch master --contains "$start" > /dev/null 2>&1
	then
		printf '%s\n' "Start hash is not in master branch"
		exit 1
	fi
fi

if [ -n "$patch" ]
then
	if ! git rev-parse stable/"$version" > /dev/null 2>&1
	then
		printf '%s\n' "Version $version does not exist"
		exit 1
	fi

	if ! [ "$patch" -ge 0 ] 2> /dev/null
	then
		printf '%s\n' "Patch $patch is not an integer"
		exit 1
	fi
else
	if git rev-parse stable/"$version" > /dev/null 2>&1
	then
		printf '%s\n' "Version $version already exists"
		exit 1
	fi
fi

if [ -z "$patch" ]
then
	revision="$version"
else
	revision="$version.$patch"
fi

if [ -z "$patch" ]
then
	if [ -z "$start" ]
	then
		git checkout -b stable/"$version"
	else
		git checkout -b stable/"$version" "$start"
	fi

	if [ "$push" = true ]
	then
		git push git@geeqie.org:geeqie stable/"$version"
	fi
else
	git checkout stable/"$version"
fi

# Regenerate to get the new version number in the man page
rm -rf build
meson setup build
ninja -C build

if ! ./scripts/generate-man-page.sh
then
	printf '%s\n' "generate-man-page.sh failed"
	exit 1
fi
if ! ./doc/create-shortcuts-xml.sh
then
	printf '%s\n' "create-shortcuts-xml.sh failed"
	exit 1
fi

git add NEWS
git add org.geeqie.Geeqie.appdata.xml.in
git add geeqie.1
git add doc/docbook/CommandLineOptions.xml
git commit --message="Preparing for release v$revision"

if [ "$push" = true ]
then
	git push git@geeqie.org:geeqie
fi

git tag --sign "v$revision" --message="Release v$revision"

if [ "$push" = true ]
then
	git push git@geeqie.org:geeqie "v$revision"
fi

rm -rf /tmp/geeqie-"$revision".tar.xz
rm -rf /tmp/geeqie-"$revision".tar.xz.asc
rm -rf build

# shellcheck disable=SC2140
tar --create --xz --file=/tmp/geeqie-"$revision".tar.xz --exclude=".git" --exclude="configure" --exclude="web" --transform s/"\bgeeqie\b"/"geeqie-$revision"/ ../geeqie
gpg --armor --detach-sign --output /tmp/geeqie-"$revision".tar.xz.asc /tmp/geeqie-"$revision".tar.xz

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

if [ "$push" = true ]
then
	git push git@geeqie.org:geeqie
fi

zenity --info --window-icon="info" --text="Upload files:\n\n/tmp/geeqie-$revision.tar.xz\n/tmp/geeqie-$revision.tar.xz.asc\n\nto https://github.com/BestImageViewer/geeqie/releases" --width=400
