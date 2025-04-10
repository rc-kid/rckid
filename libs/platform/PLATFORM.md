# Platform API

This header-only library exists to ensure that very basic embedded hardware blocks (SPI, I2C, GPIO, etc.) can be controlled in the same way across diferse platforms and to provide some general useful features (buffers, etc.).

The platform API is not meant to be comprehensive, but to provide the lowest possible common ground for uniform access across different architectures. 

- `PLATFORM_NO_STDCPP` when defined disables platform functionality depending on proper `libstdc++` implementation, that is not available on some of the embedded devices. 

- `PLATFORM_NO_STDSTRING` when the platform specifies its own string implementation (disables generating `STR` macro among other things)
