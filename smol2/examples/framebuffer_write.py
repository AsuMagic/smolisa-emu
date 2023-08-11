from smol2.asm.assembler import *

asm = Asm()

asm.at(0x0000, [
    liprel(rpl, "Literals"),

    # *0xF0002000 <- 'a'
    pl_l32(r0, 0),
    lsi(r1, ord("a")),
    s8(addr=r0, src=r1),

    Label("infvsync"),
    # wait for vsync - written value does not matter
    pl_l32(r0, 4),
    s8(addr=r0, src=r0),
    jali("infvsync"),
])

# TODO: add an Align() op
asm.at(0x0100, [
    Label("Literals"),
    (0xF0002000).to_bytes(4, byteorder="little"), # [0]
    (0xF0002FD0).to_bytes(4, byteorder="little"), # [1]

    # Label("String"),
    # "Hello, world!\0".encode("ascii")
])

if __name__ == "__main__":
    asm.to_rom()