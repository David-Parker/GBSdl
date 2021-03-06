# GBSdl
`GBSdl` is a Nintendo GameBoy emulator app for Windows/Linux/MacOS. It implements an SDL layer on top of [GBLib](https://github.com/David-Parker/GBLib).

## How to Build
Clone the repository and initialize submodule dependencies.
`git clone --recurse-submodules https://github.com/David-Parker/GBSdl.git`

In the build directory run cmake.
```
mkdir build
cd build
cmake ..
```

CMake will generate platform specific build tools, e.g. a `Makefile` on Linux.

All source and header files reside in the `src` folder.

## License
`GBSdl` is released under the MIT license.

## Running
Place your roms in the `rom` folder. The app launches a console window where you may enter a path to the ROM file. This path can also be passed as the first command line argument.

## Media

![image](images/pokemon-start.jpg)
![image](images/pokemon.jpg)
![image](images/tetris.jpg)
![image](images/dr-mario.jpg)
![image](images/blargg-cpu.jpg)