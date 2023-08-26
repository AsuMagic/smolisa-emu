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

### Configuring the CMake project

My cmake line: `cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/opt/llvm-smol2 -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_TARGETS_TO_BUILD="" -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="Smol" -DLLVM_USE_LINKER=mold -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_PARALLEL_LINK_JOBS=4 -DLLVM_ENABLE_ASSERTIONS=1 -DLLVM_USE_SPLIT_DWARF=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER="clang++" -DBUILD_SHARED_LIBS=OFF`

Breakdown:

- `-DLLVM_ENABLE_PROJECTS="clang;lld"` to build clang+lld
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo` for a build with optimizations and debug
info. Note that LLVM developers recommend using Debug, but I went with this
anyway. Bear in mind that RelWithDebInfo disables some things by default, e.g.
assertions, which are restored by other flags I specified.
- `-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="Smol"` is needed because in order to
use your target with `-DLLVM_TARGETS_TO_BUILD`, it would need to be specified as
a stable target in the CMake configuration.
- `-DLLVM_USE_LINKER=mold` (or `lld` is fine) is going to be much faster and
lightweight at linking than the stock `ld`.
- `-DLLVM_OPTIMIZED_TABLEGEN=ON` presumably makes compile times faster, I don't
know if it redundant with RelWithDebInfo builds, though.
- `-DLLVM_PARALLEL_LINK_JOBS=4` limits the number of linker jobs. Realistically
it probably could be set even lower. Linking is very memory intensive with large
debug builds so you run out very fast and hit swap otherwise.
- `-DBUILD_SHARED_LIBS=OFF` is the default. `ON` has the potential of reducing
the total binary size but I encountered a linking error I couldn't figure out.