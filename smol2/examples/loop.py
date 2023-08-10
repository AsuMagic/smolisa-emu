from smol2.asm.assembler import *

asm = Asm()

asm.at(0x0000, [
    LI(RG0, low("loop")),
    LI(RG1, 1),
    Label("loop"),
    ADD(RG2, RG2, RG1),
    OR(RIP, RG0, RG0)
])

if __name__ == "__main__":
    asm.to_rom()