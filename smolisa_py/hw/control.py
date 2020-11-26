from nmigen import *
from nmigen.sim import Simulator
from enum import Enum

RegAddrSrc = Enum('RegAddrSrc', 'R1 R2 R3 REF_IP', start=0)
RegDataSrc = Enum('RegDataSrc', 'IMM8_LOW IMM8_HIGH MEM8 MEM16 ALU', start=0)

MemAddrSrc = Enum('MemAddrSrc', 'IP REG1', start=0)
MemDataSrc = Enum('MemDataSrc', 'IP REG2', start=0)

IpSrc = Enum('IpSrc', 'ALU REG2', start=0)

AluOp = Enum('AluOp', 'ADD SUB AND OR XOR SHL SHR SWB', start=0)
AluDataSrc = Enum('AluDataSrc', 'IP REG1 REG2 TWO', start=0)

Ins = Enum('Ins', 'LI LIU LB SB LW SW LRZ LRNZ ADD SUB AND OR XOR SHL SHR SWB', start=0)
assert Ins.SWB.value == 0xF

ControlBus = [
    ("reg_we", 1),
    ("reg_waddr_src", RegAddrSrc),
    ("reg_data_src", RegDataSrc),
    ("reg_addr1_src", RegAddrSrc),
    ("reg_addr2_src", RegAddrSrc),

    ("mem_we", 1),
    ("mem_half_write", 1),
    ("mem_addr_src", MemAddrSrc),
    ("mem_data_src", MemDataSrc),

    ("alu_op", AluOp),
    ("alu_a_src", AluDataSrc),
    ("alu_b_src", AluDataSrc),
    ("alu_flag_zero", 1)
]

class Control(Elaboratable):
    def __init__(self):
        self.bus = Record(ControlBus)

        # opcode is valid from the 'exec' state.
        # on the 'exec' state it is assigned to the memory output
        # it then assigns it to 'opcode_reg' for further states still reading from it
        self.opcode = Signal(16)
        self.mem_data = Signal(16)
        self.opcode_reg = Signal(16)

        self.alu_op = Signal(AluOp)
    
    def immediate_load(self, m, data_source):
        m.d.comb += [
            self.bus.reg_we.eq(1),
            self.bus.reg_waddr_src.eq(RegAddrSrc.R1),
            self.bus.reg_data_src.eq(data_source)
        ]
    
    def exec1_li(self, m):
        self.immediate_load(m, RegDataSrc.IMM8_LOW)
        m.next = "fetch"
    
    def exec1_liu(self, m):
        self.immediate_load(m, RegDataSrc.IMM8_HIGH)
        m.next = "fetch"
    
    def exec1_lb(self, m):
        m.d.comb += self.bus.reg_addr1_src.eq(RegAddrSrc.R1)
        m.next = "exec2_lb"

    def exec2_lb(self, m):
        m.d.comb += self.bus.mem_addr_src.eq(MemAddrSrc.REG1)
        m.next = "exec3_lb"

    def exec3_lb(self, m):
        m.d.comb += [
            self.bus.reg_we.eq(1),
            self.bus.reg_waddr_src.eq(RegAddrSrc.R2),
            self.bus.reg_data_src.eq(RegDataSrc.MEM8)
        ]
        m.next = "fetch"

    def exec1_sb(self, m):
        m.d.comb += [
            self.bus.reg_addr1_src.eq(RegAddrSrc.R1),
            self.bus.reg_addr2_src.eq(RegAddrSrc.R2)
        ]
        m.next = "exec2_sb"
    
    def exec2_sb(self, m):
        m.d.comb += [
            self.bus.mem_we.eq(1),
            self.bus.mem_half_write.eq(1),
            self.bus.mem_addr_src.eq(MemAddrSrc.REG1),
            self.bus.mem_data_src.eq(MemDataSrc.REG2)
        ]
        m.next = "fetch"

    def exec1_lw(self, m):
        m.d.comb += self.bus.reg_addr1_src.eq(RegAddrSrc.R1)
        m.next = "exec2_lw"

    def exec2_lw(self, m):
        m.d.comb += self.bus.mem_addr_src.eq(MemAddrSrc.REG1)
        m.next = "exec3_lw"
    
    def exec3_lw(self, m):
        m.d.comb += [
            self.bus.reg_we.eq(1),
            self.bus.reg_waddr_src.eq(RegAddrSrc.R2),
            self.bus.reg_data_src.eq(RegDataSrc.MEM16)
        ]
        m.next = "fetch"

    def exec1_sw(self, m):
        m.d.comb += [
            self.bus.reg_addr1_src.eq(RegAddrSrc.R1),
            self.bus.reg_addr2_src.eq(RegAddrSrc.R2)
        ]
        m.next = "exec2_sb"
    
    def exec2_sw(self, m):
        m.d.comb += [
            self.bus.mem_we.eq(1),
            self.bus.mem_half_write.eq(0),
            self.bus.mem_addr_src.eq(MemAddrSrc.REG1),
            self.bus.mem_data_src.eq(MemDataSrc.REG2)
        ]
        m.next = "fetch"

    def exec1_lrz(self, m):
        m.d.comb += [
            self.bus.reg_addr1_src.eq(RegAddrSrc.R3),
            self.bus.reg_addr2_src.eq(RegAddrSrc.R2)
        ]
        m.next = "exec2_lrz"
    
    def exec2_lrz(self, m):
        m.d.comb += [
            self.bus.alu_op.eq(AluOp.SUB)
        ]

        with m.If(self.bus.alu_flag_zero):
            m.d.comb += [
                self.bus.ip_we.eq(1),
                self.bus.ip_src.eq(IpSrc.REG2)
            ]
        
        m.next = "fetch"

    def exec1_lrnz(self, m):
        m.d.comb += [
            self.bus.reg_addr1_src.eq(RegAddrSrc.R3),
            self.bus.reg_addr2_src.eq(RegAddrSrc.R2)
        ]
        m.next = "exec2_lrnz"
    
    def exec2_lrnz(self, m):
        m.d.comb += [
            self.bus.alu_op.eq(AluOp.SUB)
        ]

        with m.If(not self.bus.alu_flag_zero):
            m.d.comb += [
                self.bus.ip_we.eq(1),
                self.bus.ip_src.eq(IpSrc.REG2)
            ]
        
        m.next = "fetch"

    def exec1_alu(self, m):
        m.d.sync += self.alu_op.eq(self.opcode[0:3]) # hax
        m.d.comb += [
            self.bus.reg_addr1_src.eq(RegAddrSrc.R2),
            self.bus.reg_addr2_src.eq(RegAddrSrc.R3),
        ]
        m.next = "exec2_alu"
    
    def exec2_alu(self, m):
        m.d.comb += [
            self.bus.alu_op.eq(self.alu_op),
            self.bus.alu_a_src.eq(AluDataSrc.REG1),
            self.bus.alu_b_src.eq(AluDataSrc.REG2),
            
            self.bus.reg_we.eq(1),
            self.bus.reg_waddr_src.eq(RegAddrSrc.R1),
            self.bus.reg_data_src.eq(RegDataSrc.ALU)
        ]
        m.next = "fetch"

    def elaborate(self, platform):
        m = Module()

        ins = Signal(Ins)
        m.d.comb += ins.eq(self.opcode[0:4])

        # default, overriden in exec as it is the first cycle in which we get the opcode from mem
        m.d.comb += self.opcode.eq(self.opcode_reg)

        with m.FSM() as fsm:
            with m.State("fetch"):
                m.d.comb += [
                    self.bus.mem_addr_src.eq(MemAddrSrc.IP),

                    self.bus.reg_we.eq(1),
                    self.bus.reg_waddr_src.eq(RegAddrSrc.REF_IP),
                    self.bus.reg_data_src.eq(RegDataSrc.ALU),

                    self.bus.alu_op.eq(AluOp.ADD),
                    self.bus.alu_a_src.eq(AluDataSrc.IP),
                    self.bus.alu_b_src.eq(AluDataSrc.TWO),
                ]
                m.next = "exec"
            
            with m.State("exec"):
                m.d.comb += self.opcode.eq(self.mem_data)
                m.d.sync += self.opcode_reg.eq(self.opcode)
                
                with m.Switch(ins):
                    with m.Case(Ins.LI):
                        self.exec1_li(m)

                    with m.Case(Ins.LIU):
                        self.exec1_liu(m)
                    
                    with m.Case(Ins.LB):
                        self.exec1_lb(m)
                    
                    with m.Case(Ins.SB):
                        self.exec1_sb(m)
                    
                    with m.Case(Ins.LW):
                        self.exec1_lw(m)
                    
                    with m.Case(Ins.SW):
                        self.exec1_sw(m)
                    
                    with m.Case(Ins.LRZ):
                        self.exec1_lrz(m)
                    
                    with m.Case(Ins.LRNZ):
                        self.exec1_lrnz(m)

                    with m.Case(Ins.ADD, Ins.SUB, Ins.AND, Ins.OR, Ins.XOR, Ins.SHL, Ins.SHR, Ins.SWB):
                        self.exec1_alu(m)
            
            with m.State("exec2_lb"):
                self.exec2_lb(m)
            
            with m.State("exec3_lb"):
                self.exec3_lb(m)
            
            with m.State("exec2_sb"):
                self.exec2_sb(m)
            
            with m.State("exec2_lw"):
                self.exec2_lw(m)
            
            with m.State("exec3_lw"):
                self.exec3_lw(m)
            
            with m.State("exec2_alu"):
                self.exec2_alu(m)
            
            with m.State("exec2_lrz"):
                self.exec2_alu(m)

            with m.State("exec2_lrnz"):
                self.exec2_alu(m)
                

        return m

    def testbench(self):
        # dumb testbench, doesn't really test anything

        # fetch
        
        # li $g0 8
        yield self.mem_data.eq(Ins.LI.value | (0b0000 << 4) | (8 << 8))
        yield # exec1
        yield self.mem_data.eq(0) # not kept for more than one clock cycle
        
        # li $g1 42
        yield # fetch
        yield self.mem_data.eq(Ins.LI.value | (0b0001 << 4) | (42 << 8))
        yield # exec1
        yield self.mem_data.eq(0) # not kept for more than one clock cycle
        
        # add $g0 $g0 $g1
        yield # fetch
        yield self.mem_data.eq(Ins.ADD.value | (0b0000 << 4) | (0b0001 << 8) | (0b0001 << 12))
        yield # exec1
        yield # exec2
        yield self.mem_data.eq(0) # not kept for more than one clock cycle

        # lrnz $ip $g1 $g0
        yield # fetch
        yield self.mem_data.eq(Ins.LRNZ.value | (0b1110 << 4) | (0b0001 << 8) | (0b0000 << 12))
        yield # exec1
        yield # exec2
        yield self.mem_data.eq(0) # not kept for more than one clock cycle

dut = Control()

sim = Simulator(dut)
sim.add_clock(1e-6)
sim.add_sync_process(dut.testbench)
with sim.write_vcd("control.vcd"):
    sim.run()