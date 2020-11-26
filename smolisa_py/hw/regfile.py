from nmigen import *

class RegFile(Elaboratable):
    def __init__(self):
        self.regs = Memory(width=16, depth=16)
        
        self.wmask = Signal(2) # 0b00 - no write, 0b01 - write lower, 0b10 - write upper, 0b11 - write word
        self.waddr = Signal(4)
        self.wdata = Signal(16)

        self.raddr1 = Signal(4)
        self.raddr2 = Signal(4)

        self.rdata1 = Signal(16)
        self.rdata2 = Signal(16)
        self.rip = Signal(16)

        self.wport = self.regs.write_port(granularity=8)
        self.rport1 = self.regs.read_port(transparent=False)
        self.rport2 = self.regs.read_port(transparent=False)
        self.ripport = self.regs.read_port(transparent=True)
    
    def elaborate(self, platform):
        m = Module()

        m.submodules.wport = self.wport
        m.submodules.rport1 = self.rport1
        m.submodules.rport2 = self.rport2
        m.submodules.ripport = self.ripport

        m.d.comb += [
            self.wport.en.eq(self.wmask),
            self.wport.addr.eq(self.waddr),
            self.wport.data.eq(self.wdata),

            self.rport1.addr.eq(self.raddr1),
            self.rdata1.eq(self.rport1.data),
            
            self.rport2.addr.eq(self.raddr2),
            self.rdata2.eq(self.rport2.data),

            self.ripport.addr.eq(0xE),
            self.rip.eq(self.ripport.data)
        ]
        
        return m