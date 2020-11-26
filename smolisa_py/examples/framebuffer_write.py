from smolisa_py.asm.assembler import *

asm = Asm()

asm.at(0x0000, [
    # $bank = 0xFFFF
    LIU(RG0, 0xFF),
    LI(RG0, 0xFF),
    OR(dst=RBANK, lhs=RG0, rhs=RG0),

    # $g0 = 0x2000
    LIU(RG0, 0x20),
    LI(RG0, 0x00),

    # $g1 = 'a'
    LI(RG1, ord('a')),

    # mem[$g0] = $g1
    SB(addr=RG0, src=RG1),

    Label("infvsync"),
    # $g0 = 0x2FD0
    LIU(RG0, 0x2F),
    LI(RG0, 0xD0),

    # wait for vsync - written value does not matter
    SB(addr=RG0, src=RG0),

    LIU(RG0, high("infvsync")),
    LI(RG0, low("infvsync")),
    OR(RIP, RG0, RG0)
])

if __name__ == "__main__":
    asm.to_rom()