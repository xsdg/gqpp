# Checklist for code updates and new releases of Geeqie

## Code Updates

### Before compiling the sources, carry out the following actions when necessary

* Update `org.geeqie.Geeqie.appdata.xml.in` with the latest released version and date

* If source files have been added or removed from `./src/` directory, resync `./po/POTFILES.in`

```sh
cd ./po
./regen_potfiles.sh | patch -p0
```

* Keep translations in sync with the code

```sh
cd ./po
make update-po
```

* Update the desktop template if menus have changed

```sh
./scripts/template-desktop.sh
```

### After compiling the sources, carry out the following actions when necessary

* Update the man page and Command Line Options section in Help if the command line options have changed

```sh
./scripts/generate-man-page.sh
```

* Update the keyboard shortcuts page in Help if any keyboard shortcuts have changed

```sh
./doc/create-shortcuts-xml.sh
```

* Commit the changes and push to the .repo

* Generate a new AppImage (note that this should be run on a **20.04 system**)

```sh
./scripts/generate-appimage.sh <location of local appimages folder>
```

* Upload AppImage to web AppImages location
* Edit `<location of local geeqie.github.io>/AppImage/appimages.txt` to include latest AppImage at the *top* of the list
* Update the web-page Help files if they have changed
    * commit and push if necessary

```sh
./scripts/web-help.sh
```

* Copy `geeqie.desktop` to `<location of local geeqie.github.io>/`
* Copy `org.geeqie.Geeqie.appdata.xml` to `<location of local geeqie.github.io>/`
* Push changes to `geeqie.github.io`

## New release

Carry out the above actions to ensure the master branch is up to date, and then the following actions for new version \<n.m\>.

```sh
sudo make maintainer-clean

git checkout -b stable/<n.m>
git push git@geeqie.org:geeqie stable/<n.m>
```

Edit `org.geeqie.Geeqie.appdata.xml.in` - change date and version  
Edit `NEWS` - the usual information

```sh
./autogen.sh
make -j
./scripts/generate-man-page.sh

git add NEWS
git add org.geeqie.Geeqie.appdata.xml.in
git add geeqie.1
git add doc/docbook/CommandLineOptions.xml
git commit --message="Preparing for release v<n.m>"
git push git@geeqie.org:geeqie

git tag --sign v<n.m> --message="Release v<n.m>"
git push git@geeqie.org:geeqie v<n.m>
```

Copy the changed files from the v\<n.m\> branch to master

```sh
git checkout master

git checkout stable/<n.m> NEWS
git checkout stable/<n.m> geeqie.1
git checkout stable/<n.m> doc/docbook/CommandLineOptions.xml
git checkout stable/<n.m> org.geeqie.Geeqie.appdata.xml.in

git add NEWS
git add org.geeqie.Geeqie.appdata.xml.in
git add geeqie.1
git add doc/docbook/CommandLineOptions.xml
git commit --message="Release v<n.m> files"
git push git@geeqie.org:geeqie
```

Go to `https://github.com/BestImageViewer/geeqie/releases` and click on `Draft a new release`.

Under `Release title` insert "Geeqie \<n.m\>"

Under `Choose a tag` select `v<n.m>`

In `Describe this release` copy-paste the relevant section of `NEWS`.

Click `Publish release`
