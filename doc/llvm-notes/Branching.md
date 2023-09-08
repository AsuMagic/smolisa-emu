#### Terminators in `BasicBlock` vs `MachineBasicBlock`s
Basic blocks are linear sequences of instructions which always end with a single terminator, which describes which next branch is to be taken. The terminator can have a predicate or it can not have one.

LLVM IR can freely represent any number of operands it wants, so having a single instruction that specifies all possible target branches for that basic block is feasible (e.g. the two target basic blocks in a conditional branch, or the targets of a switch).

On the other hand, machine basic blocks aren't as free and are bound by what your actual machine instructions allow. 
As such, `MachineBasicBlock`s were designed to accommodate any number of terminators (0 or more). They are still always at the end, though, and there cannot be other things between them.
Thus you can iterate backwards in your `MachineBasicBlock` to find all terminators, though MBB's `.terminators()` also exists now.

When there is no terminator in a MBB, then the basic block will fall through the next.
Confusingly, it seems that you can consider MBB terminators as always predicated.

#### `BRCOND` vs `BR_CC`
`BR_CC` has a condition code ("`CC`") as an immediate operand, which specifies the comparison operation that should be used, followed by the two operands to compare (`brcc chain, cc, lhs, rhs, bb_taken_if_true`).

`BRCOND` instead represents the condition as a `setcc` node. In other words, `BR_CC` merges `BRCOND` and a conditional set instruction (`brcond chain, condi1, bb_taken_if_true`).

In some cases, such as if tests are performed as separate instructions from the conditional branch or set (using a hidden test flag register), matching `BRCOND` may prove more convenient. In that case, you may want to add the following in the `xxxISelLowering.cpp`, which will ensure that `brcond` gets emitted instead of not `brcc`:
```cpp
setOperationAction(ISD::BRCOND, MVT::i32, Custom);
setOperationAction(ISD::BR_CC, MVT::i32, Expand);
```
In the test register use-case, you would define the test bit as its own register and 1-register class and use pattern matching for `brcond` and `setxxx` separately, the former using a test-register-class operand specialized to the T register and the latter as outputting to that class.
#### [Branch analysis patch from `lowRISC/riscv-llvm`](<https://github.com/lowRISC/riscv-llvm/blob/master/0030-RISCV-Implement-branch-analysis.patch>)
Inside of the `RISCVInstrInfo`, this adds four *overriden* methods: `analyzeBranch`, `insertBranch`, `removeBranch`, `reverseBranchCondition`.

`analyzeBranch` can perform useful optimizations and transforms. When it **fails**, it returns `true`, which is the default for that method. If it **succeeds** in its transform, then it returns `false`.

Three machine blocks are involved at most:
- The `MBB` which is the source basic block. The end of that basic block corresponds to the branch we are analyzing.
- The `TBB` ("True" Basic Block) corresponds to the basic block that should be branched to for unconditional branches or for conditional branches when the predicate is true.
- The `FBB` ("False" Basic Block) corresponds to the basic block that should be branched to if the predicate was false.
There are more details on edge cases in the documentation.

If `analyseBranch` encounters a branch that it can "understand", then it returns `false`, preparing whatever else may be needed for that branch type, as described in the `analyzeBranch` documentation.

`analyseBranch` can also build a list of machine operands belonging to the condition. This can be done by analyzing the last instruction of the source basic block. These machine operands are arbitrary and only manipulated by the same class so you can kind of shove whatever you would need for the `insertBranch` that ensues.

Once `analyzeBranch` is done, `insertBranch` or `deleteBranch` gets called. In the `insertBranch` case, the relevant machine instructions (or pseudo instruction if convenient) for the branch gets emitted.

`reverseBranchCondition` can be provided if e.g. `>` is not supported but `<=` is after swapping operands.

Remaining Qs:
- [ ] The next patch has support for "branch relaxation"; what is this?
- [ ] How does it handle relocation and such