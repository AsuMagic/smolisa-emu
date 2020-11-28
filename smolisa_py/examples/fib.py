from smolisa_py.asm.assembler import *
import smolisa_py.asm.macros as macros

asm = Asm()

reg_first = RG0
reg_second = RG1
reg_tmp = RG2
reg_i = RG3
reg_cst_one = RG4
reg_cst_loopaddr = RG5

n = 10

asm.at(0x0000, [
    *macros.load_imm16_assume_zero(reg_first, 0),
    *macros.load_imm16_assume_zero(reg_second, 1),
    *macros.load_imm16_assume_zero(reg_tmp, 0),
    
    *macros.load_imm16_assume_zero(reg_i, n),
    
    # constants
    *macros.load_imm16_assume_zero(reg_cst_one, 1),
    *macros.load_label(reg_cst_loopaddr, "loop"),

    # loop n times (i=n, n-1, ..., 1)
    Label("loop"),
    ADD(dst=reg_tmp, lhs=reg_first, rhs=reg_second),
    *macros.load_register(dst=reg_first, src=reg_second),
    *macros.load_register(dst=reg_second, src=reg_tmp),
    SUB(dst=reg_i, lhs=reg_i, rhs=reg_cst_one),
    LRNZ(dst=RIP, src=reg_cst_loopaddr, cond=reg_i),

    # infinite loop when we're done calculating. result is in reg_first aka RG0
    *macros.load_label(RG13, "infloop"),
    Label("infloop"),
    *macros.load_register(dst=RIP, src=RG13)
])

if __name__ == "__main__":
    asm.to_rom()