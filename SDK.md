# RCKid SDK Design

> This is the software design of RCKid part and presents the generic part of the SDK.  See [HARDWARE.md](HARDWARE.md) for details about the hardware architecture and readme files in the respective app folders for their details.

RCKid sits between a complex device running an operating system and single app cartridge systems of the gaming handhelds of the past. While there is no OS and multitasking is not supported (with a few notable exceptions below), the SDK does provide 

## Backends

The SDK supports multiple backends - hardware or virtual architectures on which the cartridges running the SDK can be executed. The backend abstracts away the particular hardware details so that no code changes, only recompilation of the cartridge is necessary. 

The currently supported backends are:

- fantasy 


## SD Card Contents

The SDK expects for most of its functionality to have an SD card attached in the device. The SD card contains most of the user data, media files, etc. The following format of the SD card is expected:

    \
    |-- contacts.json
    |-- settings.json
    |-- apps
    |     |-- apps by names and their contents
    |     ...
    |
    |-- files
    |     |-- icons
    |     |-- images
    |     |-- music
    |     |-- recording
    |     ...
    |
    |-- games
    |     |-- gamefiles for emulators
    ...

### Application Folders

Each application has its own folder under the `apps` root folder. The folder has the following structure:

    AppName
       |-- data
       |-- resources
       |-- saves

The `data` folder is the app home and is completely in the application's control. The SDK does not understand what it is, nor does it  



## Simplification Ideas

The SDK only works with apps and tasks. The app is the only thing that has access to screen and it must periodically call tick() method every frame or roughly 60 times per second. The tasks on the other hand specify onTick() method, which will be called by the SDK during the tick itself. 

- have two kinds of taks - heavy and light tasks. Light tasks can run always, while heavy tasks will be killed when standalone mode requested by the app
