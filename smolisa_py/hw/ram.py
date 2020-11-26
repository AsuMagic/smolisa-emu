from nmigen import *
from nmigen.sim import Simulator

class RAM(Elaboratable):
    # TODO: $bank model handling
    def __init__(self, size, init_rom):
        self.memory = Memory(width=16, depth=size, init=init_rom)
        
        self.wmask = Signal(2)
        self.wdata = Signal(16)

        self.addr = Signal(16)
        self.rdata = Signal(16)
    
    def elaborate(self, platform):
        m = Module()

        m.submodules.rport = rport = self.memory.read_port(transparent=False)
        m.submodules.wport = wport = self.memory.write_port(granularity=8)

        real_addr = self.addr >> 1

        # handle unaligned 8-bit access
        with m.If(self.addr[0] == 1):
            m.d.comb += self.rdata[0:8].eq(rport.data[8:16])
        with m.Else():
            m.d.comb += self.rdata.eq(rport.data)

        m.d.comb += [
            rport.addr.eq(real_addr),

            wport.en.eq(self.wmask),
            wport.addr.eq(real_addr),
            wport.data.eq(self.wdata)
        ]

        return m

    def testbench(self):
        yield self.wmask.eq(0b10)
        yield self.wdata.eq(0xABCD)
        yield self.addr.eq(0x0002)
        yield

        yield self.wmask.eq(0b00)
        yield self.wdata.eq(0x0000)
        yield self.addr.eq(0x0003)
        yield
        yield
        assert (yield self.rdata) == 0xAB
        yield

dut = RAM(16, [0 for _ in range(16)])

sim = Simulator(dut)
sim.add_clock(1e-6)
sim.add_sync_process(dut.testbench)
with sim.write_vcd("ram.vcd"):
    sim.run()