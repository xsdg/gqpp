# How to generate the timezone database

FIXME Incomplete documentation

The detection of timezone is performed by
 [ZoneDetect](https://github.com/BertoldVdb/ZoneDetect)
 
 timezone16.bin has a longitude resolution of 0.0055 degrees (\~0.5km).  
 timezone21.bin has a longitude resolution of 0.00017 degrees (\~20m).
 
 The safe zone result indicates how close you are to the nearest border (flat earth model using lat and lon as x and y), so you can know when to do a new lookup. If you don't need it you can ignore it, in this case you can pass a NULL parameter to save the (very small) calculation effort.
 
 
 Note that the [underlying database](https://github.com/evansiroky/timezone-boundary-builder) will change from time-to-time.
