# Checklist for new releases of Geeqie

## Before compiling the sources, carry out the following actions when necessary

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

## After compiling the sources, carry out the following actions when necessary

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
