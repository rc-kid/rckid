# FatFS

See [here](http://elm-chan.org/fsw/ff/) for the library and documentation. This folder contains the FatFS library download with the following changes:

- CMakeLists.txt and this readme is addded
- contents of the `source` folder is moved to the `FatFS` folder for prettier include paths
- the `diskio.c` is renamed to `diskio.c.example` so that it won't get compiled
- in the `ffconf.h` configuration has been changed, see below. 

## Configuration

Done in the `ffconf.h`, namely:

- set code page to 437
- set LFN to 1 (no messing with stack or heap)
- set volumes to 1
- set LBA64 to true
- enable exFAT
