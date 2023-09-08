## Patterns

### Manual pseudo-instruction expansion
This can be implemented in `xxxAsmPrinter::emitInstruction` if `emitPseudoExpansionLowering`
### Corner cases & troubleshooting
#### Pseudo-instructions don't get expanded and don't show in the final assembly
Make sure that `if (emitPseudoExpansionLowering(*OutStreamer, MI)) { return; }` is done in `xxxAsmPrinter::emitInstruction`. 
The implementation of that function can be provided by doing:
```
#include "SmolGenMCPseudoLowering.inc"
```
This is a generated file, which can be generated with the following CMake statement:
```cmake
tablegen(LLVM xxxGenMCPseudoLowering.inc -gen-pseudo-lowering)
```

#### Emitting multiple instructions in a pattern
Pattern matching operates over the DAG and it is not possible to emit multiple unrelated instructions. You can, however, emit an instruction whose operands are the result of new instructions.

I thought I encountered this while implementing 32-bit immediate loads in an instruction set where no instruction is able to load so all at once as an immediate.
I had two instructions to aid with this: `lsiw` (load 24-bit immediate to lower bits, clear 8 upper bits), and `lsih` (load 8-bit immediate to upper bits, do not clear bits).
In this case, it is important to consider `lsih` as a read & write to the same register (for more reasons than one). Thus, the DAG node of `lsih` must accept a register as an input, which is constrained to be the same register as the output.

There may be a way to use chains to emit multiple nodes within a pattern, but I have not found whether that is achievable within TableGen.

#### Expanding a pseudo-instruction to multiple instructions
Does not seem to be supported:
```
// FIXME: This pass currently can only expand a pseudo to a single instruction.
// The pseudo expansion really should take a list of dags, not just
// a single dag, so we can do fancier things.
```
as per `addDagOperandMapping`.

It looks like this has to be implemented in C++ during lowering.

#### Expanding a pseudo-instruction using a regular Pattern
Does not seem to be supported, as it seems like it needs to be a `PseudoInstExpansion` (or a [[#Manual pseudo-instruction expansion]]).
For fancy load instructions involving special kind of value types like global addresses, see [[Relocatable symbols]].