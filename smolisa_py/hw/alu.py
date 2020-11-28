from nmigen import *
from nmigen.sim import Simulator

from control import AluOp

class ALU(Elaboratable):
    def __init__(self):
        self.a = Signal(16)
        self.b = Signal(16)
        self.op = Signal(AluOp)
        self.out = Signal(16)
        self.flag_zero = Signal(16)
    
    def elaborate(self, platform):
        m = Module()

        with m.Switch(self.op):
            with m.Case(AluOp.ADD):
                m.d.comb += self.out.eq(self.a + self.b)
            with m.Case(AluOp.SUB):
                m.d.comb += self.out.eq(self.a - self.b)
            with m.Case(AluOp.AND):
                m.d.comb += self.out.eq(self.a & self.b)
            with m.Case(AluOp.OR):
                m.d.comb += self.out.eq(self.a | self.b)
            with m.Case(AluOp.XOR):
                m.d.comb += self.out.eq(self.a ^ self.b)
            with m.Case(AluOp.SHL):
                m.d.comb += self.out.eq(self.a << self.b[0:4])
            with m.Case(AluOp.SHR):
                m.d.comb += self.out.eq(self.a >> self.b)
            with m.Case(AluOp.SWB):
                m.d.comb += [
                    self.out[0:8].eq(self.a[8:16]),
                    self.out[8:16].eq(self.b[0:8])
                ]
        
        m.d.comb += self.flag_zero.eq(self.out == 0)

        return m