# References

## Pipelined CPU design

Courtesy of george:

- http://home.ustc.edu.cn/~louwenqi/reference_books_tools/Computer%20Organization%20and%20Design%20RISC-V%20edition.pdf
- https://github.com/georgerennie/mips_sim/blob/main/src/core/core.c#L28
- https://github.com/georgerennie/mips_sim/blob/main/src/core/forwarding_unit.c

## LLVM backend

- Official LLVM guide: https://llvm.org/docs/WritingAnLLVMBackend.html#target-machine
- Blog article series on making a (very) minimal RISC-V-like backend: https://sourcecodeartisan.com/2020/09/13/llvm-backend-0.html
    - Link to the main commit that does most of the bring-up work: https://github.com/andresag01/llvm-project/commit/274cfea0f9662f0ed49f6132b0424323d0b11750#diff-c5ed19018c245547bf7fda59838e8bd5589e30bd9463930fc6ae0b2576184caa
- Didn't check yet: https://github.com/lowRISC/riscv-llvm
- Someone's "LLVM development diary", fairly short but explains a few issues that I encountered http://mups16.net/pages/llvm-development-diary.html
- Slides, describes the big picture, compile/lowering process (2014) https://llvm.org/devmtg/2014-04/PDFs/Talks/Building%20an%20LLVM%20backend.pdf
- Intro on pattern matching https://eli.thegreenplace.net/2013/02/25/a-deeper-look-into-the-llvm-code-generator-part-1

### Configuring the CMake project

My cmake line: `cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/opt/llvm-smol2 -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_TARGETS_TO_BUILD="" -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="Smol" -DLLVM_USE_LINKER=mold -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_PARALLEL_LINK_JOBS=4 -DLLVM_ENABLE_ASSERTIONS=1 -DLLVM_USE_SPLIT_DWARF=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER="clang++" -DBUILD_SHARED_LIBS=1 -DLLVM_CCACHE_BUILD=1`

Breakdown:

- `-DLLVM_ENABLE_PROJECTS="clang;lld"` to build clang+lld
- `-DCMAKE_BUILD_TYPE=Debug` for a debug build. You can use `RelWithDebInfo`,
but you will probably get worse stack traces on crash. Build types with
optimizations use less disk space (thus are faster to link) but this is mostly
mitigated by other things here.
- `-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="Smol"` is needed because in order to
use your target with `-DLLVM_TARGETS_TO_BUILD`, it would need to be specified as
a stable target in the CMake configuration.
- `-DLLVM_USE_LINKER=mold` (or `lld` is fine) is going to be much faster and
lightweight at linking than the stock `ld`.
- `-DLLVM_OPTIMIZED_TABLEGEN=ON` presumably makes compile times faster, I don't
know if it redundant with RelWithDebInfo builds, though.
- `-DLLVM_PARALLEL_LINK_JOBS=4` limits the number of linker jobs. Realistically
it probably could be set even lower. Linking is very memory intensive with large
debug builds so you run out very fast and hit swap otherwise. `mold` should also
be parallelizing in the first place, but I don't know if this flags tries to
affect this.
- `-DLLVM_USE_SPLIT_DWARF=1` splits debug info in a separate file from binaries
and object files which allegedly improve compile (link?) times a fair bit.
- `-DBUILD_SHARED_LIBS=ON` is only recommended for build environments. It
reduces disk space and link time quite significantly but you may encounter
compile errors if you didn't specify link dependencies in various CMakeLists
correctly.
- `-DLLVM_CCACHE_BUILD=1` enables ccache for the build, can help with
incremental build times.