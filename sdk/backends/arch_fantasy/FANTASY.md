# Fantasy Backend

The fantasy backend uses Raylib and provides an "emulator" of RCKid on Windows/Linux machines (likely MacOS might work too, but this is untested).


## Storage

Fantasy backend emulates the SD card and cartridge storage using the same FatFS and LittleFS filesystem drivers as the real hardware. When the app loads, it checks if files named `sd.iso` and `cartridge.iso` exist in the current working directory. If they do, they are used as the backing store of the SD card and cartridge flash respectively. The size of the files also determines the size of the respective storage. 

On Linux, a quick way of generating such files is via the dd command, i.e. the following creates a 1MB cartridge file:

    dd if=/dev/zero of=cartridge.iso bs=1K count=1024