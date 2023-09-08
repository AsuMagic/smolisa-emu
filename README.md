# smol2

Documentation and tooling for my toy "smol2" 32-bit CPU architecture.

## Quick start

- [`doc/cpu.md`: CPU & ISA documentation](doc/cpu.md)
- [`src/`: Emulator source](src/)
- [`smol2/asm/`: Source code for the Python-based assembler](smol2/asm/)
- [`smol2/examples/`: Assembly examples & demos](smol2/examples/)
- [🔗 WIP, undocumented LLVM backend](https://github.com/asumagic/llvm-project/tree/smol2)

## Compiling the emulator (*nix)

You require a recent CMake and C++20 compiler. Currently, the emulator framebuffer depends on SFML, which needs to be installed.

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
```

### Current state

Most of the CPU architecture is defined and documented at this point, though there are still moving parts, and there will probably still be quite a few changes to it.

MMIO peripherals are mostly experimental, the framebuffer is subject to change and more IO will be provided (audio, keyboard, storage).

Currently, a simple C++ emulator and a basic Python assembler are implemented.

Some basic demos run, including a [Brainfuck](https://esolangs.org/wiki/Brainfuck) interpreter and a Bad Apple demo (because, of course, I had to).

An LLVM backend is under early development.  
A hardware implementation using [Amaranth](https://github.com/amaranth-lang) (or possibly another HDL) is planned, but a C++ pipelined core emulator might happen before that.
