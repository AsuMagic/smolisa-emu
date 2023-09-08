TODO: better name
#### Lowering `GlobalAddress` (with multiple instructions)
You should define your own machine operand flags. Constraining ourselves to the Small code model as a simplification, the RISC-V backend does a few things things:
- Defining a machine operand flag enum, containing `RISCVII::MO_HI` and `RISCVII:MO_LO`
- Implementing `xxxMCExpr`, see `RISCVMCTargetExpr.h`, which is to be returned by `lowerSymbolOperand` 
- Defining one `SDNode`s for every different load instruction and machine operand flag pair involved
- Implementing `lowerGlobalAddress` to tie everything up
See:
```cpp
// Generate a sequence for accessing addresses within the first 2 GiB of
// address space. This generates the pattern (addi (lui %hi(sym)) %lo(sym)).
SDValue AddrHi = getTargetNode(N, DL, Ty, DAG, RISCVII::MO_HI);
SDValue AddrLo = getTargetNode(N, DL, Ty, DAG, RISCVII::MO_LO);
SDValue MNHi = DAG.getNode(RISCVISD::HI, DL, Ty, AddrHi);
return DAG.getNode(RISCVISD::ADD_LO, DL, Ty, MNHi, AddrLo);
```