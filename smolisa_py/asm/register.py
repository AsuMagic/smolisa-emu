class Reg:
    def __init__(self, id, name):
        self.id = id
        self.name = name

    def __repr__(self):
        return self.name

r0 = Reg(0x0, "r0")
r1 = Reg(0x1, "r1")
r2 = Reg(0x2, "r2")
r3 = Reg(0x3, "r3")
r4 = Reg(0x4, "r4")
r5 = Reg(0x5, "r5")
r6 = Reg(0x6, "r6")
r7 = Reg(0x7, "r7")
r8 = Reg(0x8, "r8")
r9 = Reg(0x9, "r9")
r10 = Reg(0xA, "r10")
r11 = Reg(0xB, "r11")
r12 = Reg(0xC, "r12")
rret = Reg(0xD, "rret")
rpl = Reg(0xE, "rpl")
rps = Reg(0xF, "rps")

regs_by_index = (
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, rret, rpl, rps
)

regs_by_name = {
    reg.name: reg for reg in regs_by_index
}