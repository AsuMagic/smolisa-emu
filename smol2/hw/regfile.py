from nmigen import *

def reg_name(id):
    if id == 0xE:
        return "ip"
    if id == 0xF:
        return "bank"
    if id == 0x10: # not a reg, just for traces
        return "(op)"

    return "g" + str(id)

class RegFile(Elaboratable):
    def __init__(self):
        self.regs = Array(Signal(16, name=f"r{i}_{reg_name(i)}") for i in range(16))
        self.rip = self.regs[0xE]
        self.rbank = self.regs[0xF]
        
        self.wmask = Signal(2) # 0b00 - no write, 0b01 - write lower, 0b10 - write upper, 0b11 - write word
        self.waddr = Signal(4)
        self.wdata = Signal(16)

        self.raddr1 = Signal(4)
        self.raddr2 = Signal(4)

        self.rdata1 = Signal(16)
        self.rdata2 = Signal(16)
    
    def elaborate(self, platform):
        m = Module()

        with m.If(self.wmask[0] == 1): # assign lower byte
            m.d.sync += self.regs[self.waddr][0:8].eq(self.wdata[0:8])
        with m.If(self.wmask[1] == 1):
            m.d.sync += self.regs[self.waddr][8:16].eq(self.wdata[8:16])
        
        m.d.sync += [
            self.rdata1.eq(self.regs[self.raddr1]),
            self.rdata2.eq(self.regs[self.raddr2])
        ]
        
        return m