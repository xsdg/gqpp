## A checklist to be used after Geeqie has been updated

Before compiling the sources, carry out the following actions when necessary:

* Update ```org.geeqie.Geeqie.appdata.xml.in``` with the latest released version and date
* If source files have been added or removed from ```.src/``` directory, resync ```./po/POTFILES.in```   
```
cd ./po  
./regen_potfiles.sh | patch -p0  
```
* Keep translations in sync with the code  
```
cd ./po  
make update-po  
```  
* Update the the timezone database  
```
./scripts/zonedetect/create_timezone_database
```

After compiling the sources, carry out the following actions when necessary:  

* Generate a new AppImage (note that this should be run on a **20.04 system**)  
```
./scripts/generate-appimage.sh <location of GeeqieWeb/appimages folder>
```
* Copy Help html files to ```<location of GeeqieWeb/help folder>```
* Copy ```geeqie.desktop``` to ```<location of GeeqieWeb/ folder>```  
* Copy ```org.geeqie.Geeqie.appdata.xml``` to ```<location of GeeqieWeb/ folder>```  
* Upload to GeeqieWeb  

