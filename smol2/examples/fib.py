from smol2.asm.assembler import *
import smol2.asm.macros as m

asm = Asm()

reg_first = r0
reg_second = r1
reg_tmp = r2
reg_i = r3

n = 10

asm.at(0x0000, [
    lsi(reg_first, 0),
    lsi(reg_second, 1),
    lsi(reg_tmp, 0),
    lsi(reg_i, n),

    # loop n times (i=n, n-1, ..., 1)
    Label("loop"),

    lr(reg_tmp, reg_first),
    iadd(reg_tmp, reg_second),
    lr(reg_first, reg_second),
    lr(reg_second, reg_tmp),

    iaddsi_tnz(reg_i, -1),
    c_ji("loop"),

    brk()
])

if __name__ == "__main__":
    asm.to_rom()
