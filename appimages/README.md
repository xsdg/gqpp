# How to create AppImages for Geeqie

## Download the required tools

Download the `linuxdeploy` tools. At the time of writing, these are:

```sh
wget -c https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
wget -c https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh
chmod +x linuxdeploy-plugin-gtk.sh
sudo apt install patchelf
sudo apt install librsvg2-dev
```

The first two of these files must be in your `$PATH` environment variable.

## Generate the executable

```sh
cd <your working area>
```

Download Geeqie sources:

```sh
git clone git://www.geeqie.org/geeqie.git
cd geeqie
```

If a run has already been made, remove any existing targets:

```sh
rm -r  <target dir>/AppDir
sudo rm -rf doc/html
```

Create a fresh target directory:

```sh
mkdir <target dir>/AppDir
```

Generate the Geeqie executable:

```sh
sudo make maintainer-clean
./autogen.sh --prefix="/usr/"
make -j
make install DESTDIR=<full path to target dir>/AppDir
```

## Generate the AppImage

```sh
cd <target dir>
linuxdeploy-x86_64.AppImage \
    --appdir ./AppDir --output appimage \
    --desktop-file ./AppDir/usr/share/applications/geeqie.desktop \
    --icon-file ./AppDir/usr/share/pixmaps/geeqie.png \
    --plugin gtk \
    --executable ./AppDir/usr/bin/geeqie`
```

## Rename AppImage

If required, rename the AppImage executable - e.g.:

```sh
mv ./Geeqie-v1.6-x86_64.AppImage $(./Geeqie-v1.6-x86_64.AppImage -v | sed 's/git//' | sed 's/-.* /-/' | sed 's/ /-v/' | sed 's/-GTK3//').AppImage
```

## Automation script

The script `./scripts/generate-appimage.sh` automates this process.
