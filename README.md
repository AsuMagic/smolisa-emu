# smolisa-tools

Assembler, emulator, documentation and demos for the smolisa architecture.

## Quick start

- [Instruction set documentation](doc/cpu.md)

### Compiling (*nix)

You require a recent CMake and C++ compiler that partially supports C++20.
The emulator framebuffer depends on SFML, which you will need installed.

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
```

### Video demo

```sh
# can be downloaded from e.g. https://marcan.st/transf/badapple.aa
example-generate-video-stream ./assets/badapple.aa > ./assets/badapple.bin

mkdir demos
smolisa-as ./examples/video.s > ./demos/video.img
smolisa-emu ./demos/video.img
```
