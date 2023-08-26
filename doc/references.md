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