librwgta
========

This repository contains GTA specific code.
The library librwgta proper has Rockstar specific plugins for librw.

Under tools/convdff and tools/convtxd there is code for some
dff and txd conversion utilities.

tools/storiesconv converts a number of file formats of GTA Liberty and Vice city
stories to standard RW files.

Under tools/IIItest you can find an ongoing project to reverse engineer GTA III.
Currently it can render the map:
![Liberty City (GTA3) rendered by IIItest](http://aap.papnet.eu/gta/screens/mapviewer1.png)

[Click here for a video](https://www.youtube.com/watch?v=6H8YP2q_-ls)

tools/storiesview is a map viewer for PS2 LCS and VCS.

tools/euryopa is a map viewer for GTA III-SA that will become an editor eventually hopefully.
Since it depends on LZO, which is GPL, consider my code in this repo dual-licenced as GPL.

# Building

Set the LIBRW environment variable to point to the librw directory.
Otherwise just like librw, i.e. premake5.
