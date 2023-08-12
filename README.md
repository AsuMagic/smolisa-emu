# smol2

Assembler, emulator, documentation and demos for the smolisa architecture.

NOTE: at this time, the README is outdated - the assembler has changed to a python implementation.

## Quick start

- [CPU & ISA documentation](doc/cpu.md)
- [Emulator source](src/)
- [Assembler source](smol2/asm/)
- [Assembly examples](smol2/examples/) (some outdated)

### Compiling (*nix)

You require a recent CMake and C++ compiler that partially supports C++20.
The emulator framebuffer depends on SFML, which you will need installed.

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
```