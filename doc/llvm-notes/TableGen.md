## Patterns

### Manual pseudo-instruction expansion
This can be implemented in `xxxAsmPrinter::emitInstruction` if `emitPseudoExpansionLowering` TODO
### Corner cases & troubleshooting
#### Debugging failed matches
When using debug mode, you might see something like:
```
ISEL: Starting selection on root node: t7: i32,ch = load<(load (s32) from %ir.arrayidx, !tbaa !3)> t0, t4, undef:i32
ISEL: Starting pattern match
  Initial Opcode index to 353
  Skipped scope entry (due to false predicate) at index 362, continuing at 378
  Skipped scope entry (due to false predicate) at index 379, continuing at 395
  Skipped scope entry (due to false predicate) at index 396, continuing at 412
  Skipped scope entry (due to false predicate) at index 413, continuing at 429
  Skipped scope entry (due to false predicate) at index 430, continuing at 446
  Skipped scope entry (due to false predicate) at index 447, continuing at 463
```
Breaking down this bit by bit:

`ISEL: Starting selection on root node: t7: i32,ch = load<(load (s32) from %ir.arrayidx, !tbaa !3)> t0, t4, undef:i32
`ISEL: Starting pattern match`
This node can be seen in the SelectionDAG under selection shortly above in the log. What these specific line tells us is that selection has begun for *this specific node*. Every step in the process of selection is then listed.
To make sense of the seemingly cryptic indices in the output, you actually need (to my knowledge) to check `xxxGenDAGISel.inc` and scroll down to `void DAGISEL_CLASS_COLONCOLON SelectCode(SDNode *N)`.
`  Initial Opcode index to 353`
Start at the `/*   362*/` comment. In my case, the line just aboves mentions `ISD::LOAD`. The code selection will iterate through, and successful or failed checks will be enumerated. In the above case...
`/* 362*/ OPC_CheckPredicate, 3, // Predicate_extload`
As listed by the log, the predicate `362` failed, which here means that the match was not an `extload`. If we look just a few lines below, I can see that this rule corresponds to `OPC_MorphNodeTo1, TARGET_VAL(Smol::L8O), 0|OPFL_Chain|OPFL_MemRefs`.

More accurately, this is the entire "scope" relevant to this particular L8O transform:
```
/* 360*/ OPC_Scope, 16, /*->378*/ // 15 children in Scope
/* 362*/ OPC_CheckPredicate, 3, // Predicate_extload
/* 364*/ OPC_CheckPredicate, 4, // Predicate_extloadi8
/* 366*/ OPC_CheckComplexPat, /*CP*/0, /*#*/1, // SelectAddrRegImmTpl<false, 6>:$ #2 #3
/* 369*/ OPC_EmitMergeInputChains1_0,
/* 370*/ OPC_MorphNodeTo1, TARGET_VAL(Smol::L8O), 0|OPFL_Chain|OPFL_MemRefs,
MVT::i32, 2/*#Ops*/, 2, 3,
// Src: (ld:{ *:[i32] } (AddrRegImmU6:{ *:[iPTR] } Rh2:{ *:[i32] }:$rs1, (imm:{ *:[i32] })<<P:Predicate_uimm6>>:$imm))<<P:Predicate_unindexedload>><<P:Predicate_extload>><<P:Predicate_extloadi8>> - Complexity = 13
// Dst: (L8O:{ *:[i32] } Rh2:{ *:[i32] }:$rs1, (imm:{ *:[i32] })<<P:Predicate_uimm6>>:$imm)
```

LLVM reads through this with a small virtual machine (or list of commands, if you prefer to view it that way), and it will skip through to the next scope if a predicate fails. When it succeeds, a graph transform is performed, as it was described in TableGen.
The last two lines are more explicit on the transform that is actually performed.
If you're wondering, the `*` seems to correspond to the HWMode -- for example for architectures with both 32-bit and 64-bit variants, where certain instructions would have different type capabilities depending on the architecture.
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